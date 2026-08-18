#pragma once

#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringRenderer.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <functional>
#include <map>

class HardwareKnobLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    HardwareKnobLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    juce::Font getLabelFont(juce::Label&) override;
};

class CorrectiveEqHardwarePanel final : public juce::Component {
public:
    CorrectiveEqHardwarePanel();
    ~CorrectiveEqHardwarePanel() override;
    void setModule(dozi::core::MasteringModulePlan*);
    void refreshFromModule();
    std::function<void()> onModuleChanged;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct BandControl {
        std::string gainKey;
        std::string frequencyKey;
        std::string qKey;
        juce::Slider gain;
        juce::Label name;
        juce::Label frequency;
    };
    void refresh();
    void selectBand(std::size_t);
    void refreshSelectedBand();
    void updateBypass();
    void restoreAnalysisSettings();
    HardwareKnobLookAndFeel knobLookAndFeel;
    juce::Image faceplateSkin;
    juce::TextButton inButton {"IN"};
    juce::TextButton autoButton {"AUTO"},resetButton {"RESET"};
    juce::TextButton bandInButton {"BAND IN"},resetBandButton {"RESET BAND"};
    juce::Label title,status,selectedBandLabel;
    std::array<juce::Label,3> detailLabels;
    std::array<juce::Slider,3> detailControls;
    std::array<BandControl,4> bands;
    std::size_t selectedBand = 0;
    bool updatingControls = false;
    dozi::core::MasteringModulePlan* module = nullptr;
    dozi::core::MasteringModulePlan* lastModule = nullptr;
    std::map<std::string,double> analysisSettings;
    std::map<std::string,double> styleSettings;
};

class SkinnedProcessorHardwarePanel final : public juce::Component {
public:
    SkinnedProcessorHardwarePanel();
    ~SkinnedProcessorHardwarePanel() override;
    void setModule(dozi::core::MasteringModulePlan*);
    void refreshFromModule();
    void setTelemetry(std::optional<dozi::core::ProcessorTelemetry>);
    void setAvailableTelemetry(const std::vector<dozi::core::ProcessorTelemetry>&);
    std::function<void()> onModuleChanged;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct Control { std::string key; std::unique_ptr<juce::Slider> slider; };
    struct Switch { std::string key; double onValue=1,offValue=0; std::unique_ptr<juce::TextButton> button; };
    void rebuild();
    void applyAnalysisSettings();
    void applyStyleSettings();
    void paintProcessorDisplay(juce::Graphics&);
    [[nodiscard]] double parameter(const std::string&, double fallback = 0.0) const;
    HardwareKnobLookAndFeel lookAndFeel;
    juce::Image skin;
    juce::TextButton inButton {"IN"},autoButton {"AUTO"},resetButton {"RESET"};
    juce::TextButton globalListenButton {"LISTEN"},hqButton {"HQ"};
    std::array<juce::TextButton,3> characterButtons {{juce::TextButton{"CLEAN"},juce::TextButton{"WARM"},juce::TextButton{"RICH"}}};
    std::vector<Control> controls;
    std::vector<Switch> switches;
    dozi::core::MasteringModulePlan* module = nullptr;
    std::map<std::string,double> analysisSettings;
    std::map<std::string,double> styleSettings;
    std::optional<dozi::core::ProcessorTelemetry> telemetry;
    std::vector<dozi::core::ProcessorTelemetry> availableTelemetry;
};
