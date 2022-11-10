#pragma once


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
    AudioParameterFloat* comb1_delay_time;
    AudioParameterFloat* comb1_gain;
    AudioParameterFloat* comb2_delay_time;
    AudioParameterFloat* comb2_gain;
    AudioParameterFloat* comb3_delay_time;
    AudioParameterFloat* comb3_gain;
    AudioParameterFloat* comb4_delay_time;
    AudioParameterFloat* comb4_gain;

    AudioParameterFloat* allpass1_delay_time;
    AudioParameterFloat* allpass1_gain;
    AudioParameterFloat* allpass2_delay_time;
    AudioParameterFloat* allpass2_gain;
    AudioParameterFloat* allpass3_delay_time;
    AudioParameterFloat* allpass3_gain;

    AudioParameterFloat* dry;
    AudioParameterFloat* wet;

    size_t               comb_line_count = 4;
    float**              comb_lines;
    size_t*              comb_heads;

    size_t               allpass_line_count = 3;
    float***             allpass_lines;
    size_t*              allpass_heads;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlugin)
};