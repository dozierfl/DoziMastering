#pragma once

#include "dozi/core/MasteringDecisionEngine.h"
#include "MasteringHardwareControls.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <memory>
#include <vector>

class MasteringModuleInspector final : public juce::Component {
public:
    MasteringModuleInspector();
    void setPlan(dozi::core::MasteringPlan*);
    void setTelemetry(const std::vector<dozi::core::ProcessorTelemetry>&);
    void updateTelemetry(const dozi::core::ProcessorTelemetry&);
    std::function<void()> onPlanChanged;
    void resized() override;
private:
    struct ParameterControl { std::string key; std::unique_ptr<juce::Label> label; std::unique_ptr<juce::Slider> slider; };
    void rebuildModules();
    void rebuildParameters();
    void showHardware(bool, bool rememberPreference = true);
    juce::ComboBox moduleSelector;
    juce::TextButton hardwareButton {"Hardware"},technicalButton {"Technical"};
    juce::ToggleButton enabledButton {"Module Enabled"};
    juce::Component hardwareCanvas;
    CorrectiveEqHardwarePanel correctiveEqHardware;
    SkinnedProcessorHardwarePanel processorHardware;
    juce::Viewport viewport;
    juce::Component parameterContent;
    std::vector<ParameterControl> controls;
    dozi::core::MasteringPlan* plan = nullptr;
    bool hardwareVisible = true;
    bool hardwarePreferred = true;
    std::vector<dozi::core::ProcessorTelemetry> telemetry;
};
