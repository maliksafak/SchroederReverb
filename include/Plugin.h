#pragma once

#define COMB_FILTER_COUNT 10
#define ALLPASS_FILTER_COUNT 8

//==============================================================================
class AudioPlugin : public AudioProcessor {
  public:
    //==============================================================================
    AudioPlugin();

    //==============================================================================
    void prepareToPlay(double, int) override;
    void releaseResources() override;

    template <typename T>
    void processSamples(AudioBuffer<T>& audioBuffer, MidiBuffer& midiBuffer);
    void processBlock(AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) override;
    void processBlock(AudioBuffer<double>& audioBuffer, MidiBuffer& midiBuffer) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override { return new GenericAudioProcessorEditor(*this); }
    bool                  hasEditor() const override { return true; }

    //==============================================================================
    const String getName() const override { return "Schroeder Reverb PlugIn"; }
    bool         acceptsMidi() const override { return false; }
    bool         producesMidi() const override { return false; }
    double       getTailLengthSeconds() const override { return 0; }

    //==============================================================================
    int          getNumPrograms() override { return 1; }
    int          getCurrentProgram() override { return 0; }
    void         setCurrentProgram(int) override {}
    const String getProgramName(int) override { return "None"; }
    void         changeProgramName(int, const String&) override {}

    //==============================================================================
    void getStateInformation(MemoryBlock& destData) override;

    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
        const auto& mainInLayout  = layouts.getChannelSet(true, 0);
        const auto& mainOutLayout = layouts.getChannelSet(false, 0);

        return (mainInLayout == mainOutLayout && (!mainInLayout.isDisabled()));
    }

  private:
    //==============================================================================
    AudioParameterFloat** comb_delay_time;
    AudioParameterFloat** comb_gain;

    AudioParameterFloat** allpass_delay_time;
    AudioParameterFloat** allpass_gain;

    AudioParameterFloat* dry;
    AudioParameterFloat* wet;

    float**              comb_lines;
    size_t*              comb_heads;

    float***             allpass_lines;
    size_t*              allpass_heads;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlugin)
};