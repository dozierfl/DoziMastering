#pragma once

#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringRenderer.h"
#include "MasteringHardwareControls.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <functional>
#include <optional>

class GlueCompressorHardwarePanel final : public juce::Component {
public:
    GlueCompressorHardwarePanel();
    ~GlueCompressorHardwarePanel() override;
    void setModule(dozi::core::MasteringModulePlan*);
    void setTelemetry(std::optional<dozi::core::ProcessorTelemetry>);
    void setLiveMetering(double inputDbfs, double outputDbfs, double gainReductionDb, bool active);
    void refreshFromModule();
    std::function<void()> onModuleChanged;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class LookAndFeel final : public juce::LookAndFeel_V4 {
    public:
        void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
        void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
        void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
        void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    } lookAndFeel;
    void configure(juce::Slider&,const char*,double,double,double,const char*,double);
    void write(const char*,double);
    [[nodiscard]] double read(const char*,double) const;
    void paintLedMeter(juce::Graphics&,juce::Rectangle<float>,double,const juce::String&);
    void paintVu(juce::Graphics&,juce::Rectangle<float>,double,const juce::String&);
    dozi::core::MasteringModulePlan* module=nullptr;
    std::optional<dozi::core::ProcessorTelemetry> telemetry;
    double liveInputDbfs=-100.0,liveOutputDbfs=-100.0,liveGainReductionDb=0.0;
    bool liveMeteringActive=false;
    PrimaryDragSlider threshold,attack,release,ratio,makeup;
    juce::ToggleButton inButton,autoButton,autoReleaseButton,hpfButton;
    juce::TextButton resetButton{"RESET"};
    bool refreshing=false;
};
