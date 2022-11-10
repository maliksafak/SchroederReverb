#include <JuceHeader.h>
#include "Plugin.h"

AudioPlugin::AudioPlugin()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", AudioChannelSet::stereo())
                         .withOutput("Output", AudioChannelSet::stereo())) {
    addParameter(
        comb1_delay_time = new AudioParameterFloat(
            {"comb1_delay_time", 1}, "Comb 1 Delay Time", 1.0f, 100.0f, 67.48f
        )
    );
    addParameter(
        comb1_gain = new AudioParameterFloat(
            {"comb1_gain", 1}, "Comb 1 Gain", 0.01f, 1.0f, 0.773f
        )
    );
    addParameter(
        comb2_delay_time = new AudioParameterFloat(
            {"comb2_delay_time", 1}, "Comb 2 Delay Time", 1.0f, 100.0f, 64.04f
        )
    );
    addParameter(
        comb2_gain = new AudioParameterFloat(
            {"comb2_gain", 1}, "Comb 2 Gain", 0.01f, 1.0f, 0.802f
        )
    );
    addParameter(
        comb3_delay_time = new AudioParameterFloat(
            {"comb3_delay_time", 1}, "Comb 3 Delay Time", 1.0f, 100.0f, 82.12f
        )
    );
    addParameter(
        comb3_gain = new AudioParameterFloat(
            {"comb3_gain", 1}, "Comb 3 Gain", 0.01f, 1.0f, 0.753f
        )
    );
    addParameter(
        comb4_delay_time = new AudioParameterFloat(
            {"comb4_delay_time", 1}, "Comb 4 Delay Time", 1.0f, 100.0f, 90.04f
        )
    );
    addParameter(
        comb4_gain = new AudioParameterFloat(
            {"comb4_gain", 1}, "Comb 4 Gain", 0.01f, 1.0f, 0.733f
        )
    );

    addParameter(
        allpass1_delay_time = new AudioParameterFloat(
            {"allpass1_delay_time", 1}, "AllPass 1 Delay Time", 1.0f, 100.0f, 13.88f
        )
    );
    addParameter(
        allpass1_gain = new AudioParameterFloat(
            {"allpass1_gain", 1}, "AllPass 1 Gain", 0.0f, 1.0f, 0.7f
        )
    );
    addParameter(
        allpass2_delay_time = new AudioParameterFloat(
            {"allpass2_delay_time", 1}, "AllPass 2 Delay Time", 1.0f, 100.0f, 4.52f
        )
    );
    addParameter(
        allpass2_gain = new AudioParameterFloat(
                {"allpass2_gain", 1}, "AllPass 2 Gain", 0.0f, 1.0f, 0.7f
            )
    );
    addParameter(
        allpass3_delay_time = new AudioParameterFloat(
            {"allpass3_delay_time", 1}, "AllPass 3 Delay Time", 1.0f, 100.0f, 1.48f
        )
    );
    addParameter(
        allpass3_gain = new AudioParameterFloat(
            {"allpass3_gain", 1}, "AllPass 3 Gain", 0.0f, 1.0f, 0.7f
        )
    );

    addParameter(dry = new AudioParameterFloat({"dry", 1}, "Dry", 0.0f, 1.0f, 1.0f));
    addParameter(wet = new AudioParameterFloat({"wet", 1}, "Wet", 0.0f, 1.0f, 0.6f));
}

void AudioPlugin::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    comb_lines    = new float*[comb_line_count];
    comb_heads    = new size_t[comb_line_count];
    allpass_lines = new float**[allpass_line_count];
    allpass_heads = new size_t[allpass_line_count];

    for (size_t line = 0; line < comb_line_count; line++) {
        comb_lines[line] = new float[(size_t)sampleRate];
        for (size_t sample = 0; sample < sampleRate; sample++)
        {
            comb_lines[line][sample] = 0.0f;
        }

        comb_heads[line] = 0;
    }
    for (size_t line = 0; line < allpass_line_count; line++) {
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
        
        float comb1 = comb_filter(sample_dry, comb_lines[0], &comb_heads[0], comb1_delay_time->get(), comb1_gain->get(), sample_rate);
        float comb2 = comb_filter(sample_dry, comb_lines[1], &comb_heads[1], comb2_delay_time->get(), comb2_gain->get(), sample_rate);
        float comb3 = comb_filter(sample_dry, comb_lines[2], &comb_heads[2], comb3_delay_time->get(), comb3_gain->get(), sample_rate);
        float comb4 = comb_filter(sample_dry, comb_lines[3], &comb_heads[3], comb4_delay_time->get(), comb4_gain->get(), sample_rate);
        float sample_wet = comb1 + comb2 + comb3 + comb4;

        sample_wet = allpass_filter(sample_wet, allpass_lines[0], &allpass_heads[0], allpass1_delay_time->get(), allpass1_gain->get(), sample_rate);
        sample_wet = allpass_filter(sample_wet, allpass_lines[1], &allpass_heads[1], allpass1_delay_time->get(), allpass1_gain->get(), sample_rate);
        sample_wet = allpass_filter(sample_wet, allpass_lines[2], &allpass_heads[2], allpass1_delay_time->get(), allpass1_gain->get(), sample_rate);

        for (size_t channel = 0; channel < (size_t)audioBuffer.getNumChannels(); channel++)
        {
            writer[channel][sample] = (T)(sample_wet / 8.0f);
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
    memoryOutputStream->writeFloat(*comb1_delay_time);
    memoryOutputStream->writeFloat(*comb1_gain);
    memoryOutputStream->writeFloat(*comb2_delay_time);
    memoryOutputStream->writeFloat(*comb2_gain);
    memoryOutputStream->writeFloat(*comb3_delay_time);
    memoryOutputStream->writeFloat(*comb3_gain);
    memoryOutputStream->writeFloat(*comb4_delay_time);
    memoryOutputStream->writeFloat(*comb4_gain);

    memoryOutputStream->writeFloat(*allpass1_delay_time);
    memoryOutputStream->writeFloat(*allpass1_gain);
    memoryOutputStream->writeFloat(*allpass2_delay_time);
    memoryOutputStream->writeFloat(*allpass2_gain);
    memoryOutputStream->writeFloat(*allpass3_delay_time);
    memoryOutputStream->writeFloat(*allpass3_gain);

    memoryOutputStream->writeFloat(*dry);
    memoryOutputStream->writeFloat(*wet);
    delete memoryOutputStream;
}

void AudioPlugin::setStateInformation(const void* data, int sizeInBytes) {
    MemoryInputStream* memoryInputStream =
        new MemoryInputStream(data, static_cast<size_t>(sizeInBytes), false);
    comb1_delay_time    ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    comb1_gain          ->setValueNotifyingHost(memoryInputStream->readFloat());
    comb2_delay_time    ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    comb2_gain          ->setValueNotifyingHost(memoryInputStream->readFloat());
    comb3_delay_time    ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    comb3_gain          ->setValueNotifyingHost(memoryInputStream->readFloat());
    comb4_delay_time    ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    comb4_gain          ->setValueNotifyingHost(memoryInputStream->readFloat());

    allpass1_delay_time ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    allpass1_gain       ->setValueNotifyingHost(memoryInputStream->readFloat());
    allpass2_delay_time ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    allpass2_gain       ->setValueNotifyingHost(memoryInputStream->readFloat());
    allpass3_delay_time ->setValueNotifyingHost(memoryInputStream->readFloat() / 100.0f);
    allpass3_gain       ->setValueNotifyingHost(memoryInputStream->readFloat());

    wet                 ->setValueNotifyingHost(memoryInputStream->readFloat());
    dry                 ->setValueNotifyingHost(memoryInputStream->readFloat());
    delete memoryInputStream;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AudioPlugin(); }