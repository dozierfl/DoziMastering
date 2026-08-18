#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class DoziGlueLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
};

class DoziGlueCompressorEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit DoziGlueCompressorEditor(DoziGlueCompressorProcessor&);
    ~DoziGlueCompressorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void paintLedMeter(juce::Graphics&,juce::Rectangle<float>,float,const juce::String&);
    void paintVu(juce::Graphics&,juce::Rectangle<float>,float,const juce::String&);
    void configureKnob(juce::Slider&,const char*,const char*);
    DoziGlueCompressorProcessor& glueProcessor;
    DoziGlueLookAndFeel lookAndFeel;
    juce::Slider threshold,attack,release,ratio,makeup;
    juce::ToggleButton inButton,autoButton,autoReleaseButton,hpfButton;
    juce::TextButton resetButton {"RESET"};
    using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment=juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::array<std::unique_ptr<SliderAttachment>,5> sliderAttachments;
    std::array<std::unique_ptr<ButtonAttachment>,4> buttonAttachments;
    GlueCompressorLevels displayed;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DoziGlueCompressorEditor)
};
