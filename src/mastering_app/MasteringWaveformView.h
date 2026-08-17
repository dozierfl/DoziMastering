#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <vector>

class MasteringWaveformView final : public juce::Component {
public:
    void setWaveform(std::vector<float>, double durationSeconds);
    void setPlayhead(double seconds);
    void setHorizontalInset(float pixels);
    void setSelection(double startSeconds, double endSeconds);
    void clearSelection();
    [[nodiscard]] bool hasSelection() const;
    [[nodiscard]] double selectionStartSeconds() const;
    [[nodiscard]] double selectionEndSeconds() const;
    std::function<void(double)> onSeek;
    std::function<void(double,double)> onSelectionChanged;
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    [[nodiscard]] double secondsAt(int x) const;
    std::vector<float> waveform;
    double duration = 0, playhead = 0, anchor = 0, selectionStart = 0, selectionEnd = 0;
    float horizontalInset = 0.0f;
    bool dragging = false;
};
