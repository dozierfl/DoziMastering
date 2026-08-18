#pragma once

#include "GlueCompressorEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class DoziGlueCompressorProcessor final : public juce::AudioProcessor {
public:
    DoziGlueCompressorProcessor();
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override{return true;}
    const juce::String getName() const override{return JucePlugin_Name;}
    bool acceptsMidi() const override{return false;}
    bool producesMidi() const override{return false;}
    bool isMidiEffect() const override{return false;}
    double getTailLengthSeconds() const override{return 0;}
    int getNumPrograms() override{return 1;}
    int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{}
    const juce::String getProgramName(int) override{return {};}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    [[nodiscard]] static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void resetParametersToDefaults();
    [[nodiscard]] GlueCompressorLevels meterSnapshot() const;
    juce::AudioProcessorValueTreeState parameters;
private:
    GlueCompressorEngine engine;
    std::array<std::atomic<float>,6> meters {{-100,-100,-100,-100,0,0}};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoziGlueCompressorProcessor)
};
