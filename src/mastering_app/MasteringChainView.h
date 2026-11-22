#pragma once

#include "MasteringModuleInspector.h"
#include <array>
#include <memory>

class MasteringChainView final : public juce::Component {
public:
    MasteringChainView();
    ~MasteringChainView() override;
    void setPlan(dozi::core::MasteringPlan*);
    void refreshFromPlan();
    void setTelemetry(const std::vector<dozi::core::ProcessorTelemetry>&);
    void updateTelemetry(const dozi::core::ProcessorTelemetry&);
    void updateGlueLiveMetering(double inputDbfs,double outputDbfs,double gainReductionDb,bool active);
    void openProcessorEditor(dozi::core::MasteringModuleType);
    void closeAllEditors();
    [[nodiscard]] std::size_t openEditorCount() const;
    [[nodiscard]] static constexpr std::size_t editorCapacity() { return 8; }
    [[nodiscard]] juce::Rectangle<int> processorBounds(std::size_t) const;
    [[nodiscard]] juce::Component* processorPanel(std::size_t) const;
    bool handleProcessorWindowShortcut(const juce::KeyPress&);
    std::function<void()> onPlanChanged;
    std::function<bool(const juce::KeyPress&)> onTransportShortcut;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class ProcessorCard;
    class ProcessorWindow;
    void openProcessor(std::size_t);
    dozi::core::MasteringPlan* plan = nullptr;
    std::vector<dozi::core::ProcessorTelemetry> telemetry;
    std::array<std::unique_ptr<ProcessorCard>,8> cards;
    std::array<std::unique_ptr<ProcessorWindow>,8> windows;
};
