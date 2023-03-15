#include <JuceHeader.h>
#include "Plugin.h"

AudioPlugin::AudioPlugin()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", AudioChannelSet::stereo())
                         .withOutput("Output", AudioChannelSet::stereo())) {
    
    // Comb
    comb_delay_time = new AudioParameterFloat*[COMB_FILTER_COUNT];
    comb_gain = new AudioParameterFloat*[COMB_FILTER_COUNT];

    for (size_t comb = 0; comb < COMB_FILTER_COUNT; comb++)
    {
        // Comb Delay Time
        char time_id_buffer[] = "comb00_delay_time";
        auto time_id_size = sizeof(time_id_buffer);

        char time_name_buffer[] = "Comb 00 Delay Time";
        auto time_name_size = sizeof(time_name_buffer);

        std::snprintf(time_id_buffer, time_id_size, "comb%02zu_delay_time", comb);
        std::snprintf(time_name_buffer, time_name_size, "Comb %02zu Delay Time", comb);

        comb_delay_time[comb] = new AudioParameterFloat(
            {time_id_buffer, 1}, time_name_buffer, 1.0f, 100.0f, 1.0f
        );

        addParameter(comb_delay_time[comb]);

        // Comb Gain
        char gain_id_buffer[] = "comb00_gain";
        auto gain_id_size = sizeof(gain_id_buffer);

        char gain_name_buffer[] = "Comb 00 Gain";
        auto gain_name_size = sizeof(gain_name_buffer);

        std::snprintf(gain_id_buffer, gain_id_size, "comb%02zu_gain", comb);
        std::snprintf(gain_name_buffer, gain_name_size, "Comb %02zu Gain", comb);

        comb_gain[comb] = new AudioParameterFloat(
            {gain_id_buffer, 1}, gain_name_buffer, 0.01f, 1.0f, 0.01f
        );

        addParameter(comb_gain[comb]);
    }

    // Allpass
    allpass_delay_time = new AudioParameterFloat*[ALLPASS_FILTER_COUNT];
    allpass_gain = new AudioParameterFloat*[ALLPASS_FILTER_COUNT];

    for (size_t allpass = 0; allpass < ALLPASS_FILTER_COUNT; allpass++)
    {
        // Allpass Delay Time
        char time_id_buffer[] = "allpass00_delay_time";
        auto time_id_size = sizeof(time_id_buffer);

        char time_name_buffer[] = "Allpass 00 Delay Time";
        auto time_name_size = sizeof(time_name_buffer);

        std::snprintf(time_id_buffer, time_id_size, "allpass%02zu_delay_time", allpass);
        std::snprintf(time_name_buffer, time_name_size, "Allpass %02zu Delay Time", allpass);

        allpass_delay_time[allpass] = new AudioParameterFloat(
            {time_id_buffer, 1}, time_name_buffer, 1.0f, 100.0f, 1.0f
        );

        addParameter(allpass_delay_time[allpass]);

        // Allpass Gain
        char gain_id_buffer[] = "allpass00_gain";
        auto gain_id_size = sizeof(gain_id_buffer);

        char gain_name_buffer[] = "Allpass 00 Gain";
        auto gain_name_size = sizeof(gain_name_buffer);

        std::snprintf(gain_id_buffer, gain_id_size, "allpass%02zu_gain", allpass);
        std::snprintf(gain_name_buffer, gain_name_size, "Allpass %02zu Gain", allpass);

        allpass_gain[allpass] = new AudioParameterFloat(
            {gain_id_buffer, 1}, gain_name_buffer, 0.01f, 1.0f, 0.01f
        );

        addParameter(allpass_gain[allpass]);
    }

    addParameter(dry = new AudioParameterFloat({"dry", 1}, "Dry", 0.0f, 1.0f, 1.0f));
    addParameter(wet = new AudioParameterFloat({"wet", 1}, "Wet", 0.0f, 1.0f, 0.6f));
}

void AudioPlugin::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    comb_lines    = new float*[COMB_FILTER_COUNT];
    comb_heads    = new size_t[COMB_FILTER_COUNT];
    allpass_lines = new float**[ALLPASS_FILTER_COUNT];
    allpass_heads = new size_t[ALLPASS_FILTER_COUNT];

    for (size_t line = 0; line < COMB_FILTER_COUNT; line++) {
        comb_lines[line] = new float[(size_t)sampleRate];
        for (size_t sample = 0; sample < sampleRate; sample++)
        {
            comb_lines[line][sample] = 0.0f;
        }

        comb_heads[line] = 0;
    }
    for (size_t line = 0; line < ALLPASS_FILTER_COUNT; line++) {
        allpass_lines[line]    = new float*[2];
        allpass_lines[line][0] = new float[(size_t)sampleRate];
        allpass_lines[line][1] = new float[(size_t)sampleRate];
        for (size_t sample = 0; sample < sampleRate; sample++)
        {
            allpass_lines[line][0][sample] = 0.0f;
            allpass_lines[line][1][sample] = 0.0f;
        }

        allpass_heads[line] = 0;
    }
}

void AudioPlugin::releaseResources() {
    // for (size_t line = 0; line < comb_line_count; line++) {
    //     delete[] comb_lines[line];
    // }
    // for (size_t line = 0; line < allpass_line_count; line++) {
    //     delete[] allpass_lines[line][0];
    //     delete[] allpass_lines[line][1];
    // }
    // for (size_t line = 0; line < allpass_line_count; line++) {
    //     delete[] allpass_lines[line];
    // }

    // delete[] comb_lines;
    // delete[] comb_heads;
    // delete[] allpass_lines;
    // delete[] allpass_heads;
}

inline float comb_filter(float sample_dry, float* buffer, size_t* play_head, float delay_time, float gain, double sample_rate) {
    size_t delay_time_in_samples = (size_t)((double)delay_time * (sample_rate / 1000.0));
    float sample_wet = sample_dry + (gain * buffer[*(play_head)]);
    buffer[*(play_head)] = sample_wet;
    *(play_head) = (*(play_head) + 1) % delay_time_in_samples;
    return sample_wet;
}

inline float allpass_filter(float sample_dry, float** buffer, size_t* play_head, float delay_time, float gain, double sample_rate) {
    size_t delay_time_in_samples = (size_t)((double)delay_time * (sample_rate / 1000.0));
    float sample_wet = ((-gain) * sample_dry) + buffer[0][*(play_head)] + (gain * buffer[1][*(play_head)]);
    buffer[0][*(play_head)] = sample_dry;
    buffer[1][*(play_head)] = sample_wet;
    *(play_head) = (*(play_head) + 1) % delay_time_in_samples;
    return sample_wet;
}

template <typename T>
void AudioPlugin::processSamples(AudioBuffer<T>& audioBuffer, MidiBuffer& midiBuffer) {
    auto reader = audioBuffer.getArrayOfReadPointers();
    auto writer = audioBuffer.getArrayOfWritePointers();

    double sample_rate = getSampleRate();

    for (size_t sample = 0; sample < (size_t)audioBuffer.getNumSamples(); sample++) {
        float sample_dry = 0.0;
        for (size_t channel = 0; channel < (size_t)audioBuffer.getNumChannels(); channel++)
        {
            sample_dry += (float)reader[channel][sample];
        }

        float sample_wet = 0.0f;
        for (size_t comb = 0; comb < COMB_FILTER_COUNT; comb++)
        {
            sample_wet += comb_filter(sample_dry, comb_lines[comb], &comb_heads[comb], comb_delay_time[comb]->get(), comb_gain[comb]->get(), sample_rate);
        }
        
        for (size_t allpass = 0; allpass < ALLPASS_FILTER_COUNT; allpass++)
        {
            sample_wet = allpass_filter(sample_wet, allpass_lines[allpass], &allpass_heads[allpass], allpass_delay_time[allpass]->get(), allpass_gain[allpass]->get(), sample_rate);
        }

        for (size_t channel = 0; channel < (size_t)audioBuffer.getNumChannels(); channel++)
        {
            writer[channel][sample] = (T)(sample_wet / (float)(COMB_FILTER_COUNT + ALLPASS_FILTER_COUNT));
        }
        
    }
}

void AudioPlugin::processBlock(AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) {
    processSamples<float>(audioBuffer, midiBuffer);
}

void AudioPlugin::processBlock(AudioBuffer<double>& audioBuffer, MidiBuffer& midiBuffer) {
    processSamples<double>(audioBuffer, midiBuffer);
}

void AudioPlugin::getStateInformation(MemoryBlock& destData) {
    MemoryOutputStream* memoryOutputStream = new MemoryOutputStream(destData, true);
    for (size_t comb = 0; comb < COMB_FILTER_COUNT; comb++)
    {
        memoryOutputStream->writeFloat(*comb_delay_time[comb]);
        memoryOutputStream->writeFloat(*comb_gain[comb]);
    }
    for (size_t allpass = 0; allpass < ALLPASS_FILTER_COUNT; allpass++)
    {
        memoryOutputStream->writeFloat(*allpass_delay_time[allpass]);
        memoryOutputStream->writeFloat(*allpass_gain[allpass]);
    }

    memoryOutputStream->writeFloat(*dry);
    memoryOutputStream->writeFloat(*wet);
    delete memoryOutputStream;
}

void AudioPlugin::setStateInformation(const void* data, int sizeInBytes) {
    MemoryInputStream* memoryInputStream =
        new MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false);
    for (size_t comb = 0; comb < COMB_FILTER_COUNT; comb++)
    {
        comb_delay_time[comb]    ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
        comb_gain[comb]          ->setValueNotifyingHost(memoryInputStream->readFloat());
    }

    for (size_t allpass = 0; allpass < ALLPASS_FILTER_COUNT; allpass++)
    {
        allpass_delay_time[allpass]    ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
        allpass_gain[allpass]          ->setValueNotifyingHost(memoryInputStream->readFloat());
    }

    wet                 ->setValueNotifyingHost(memoryInputStream->readFloat());
    dry                 ->setValueNotifyingHost(memoryInputStream->readFloat());
    delete memoryInputStream;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AudioPlugin(); }