#pragma once

#include "dozi/core/MasteringDecisionEngine.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <memory>
#include <vector>

class MasteringModuleInspector final : public juce::Component {
public:
    MasteringModuleInspector();
    void setPlan(dozi::core::MasteringPlan*);
    std::function<void()> onPlanChanged;
    void resized() override;
private:
    struct ParameterControl { std::string key; std::unique_ptr<juce::Label> label; std::unique_ptr<juce::Slider> slider; };
    void rebuildModules();
    void rebuildParameters();
    juce::ComboBox moduleSelector;
    juce::ToggleButton enabledButton {"Module Enabled"};
    juce::Viewport viewport;
    juce::Component parameterContent;
    std::vector<ParameterControl> controls;
    dozi::core::MasteringPlan* plan = nullptr;
};
