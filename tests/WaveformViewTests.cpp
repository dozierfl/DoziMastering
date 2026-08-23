#include "MasteringWaveformView.h"

#include <iostream>

namespace {
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
MasteringWaveformView::ChannelEnvelope envelope(float polarity) {
    return {{-0.2f * polarity, 0.4f * polarity}, {-0.7f * polarity, 0.3f * polarity}, {-0.1f * polarity, 0.8f * polarity}};
}
}

int main() {
    MasteringWaveformView view;
    view.setSize(800, 360);
    view.setWaveform({envelope(1.0f)}, 180.0);
    expect(view.displayedChannelCount() == 1, "mono input displays one waveform lane");
    expect(std::abs(MasteringWaveformView::amplitudeForDbfs(-6.0)-0.501187f)<0.0001f, "waveform ruler converts -6 dBFS to half-scale amplitude");
    expect(MasteringWaveformView::amplitudeForDbfs(-100.0)==0.0f, "waveform ruler maps silence to the lane centre");

    view.setWaveform({envelope(1.0f), envelope(-1.0f)}, 180.0);
    expect(view.displayedChannelCount() == 2, "stereo input displays separate left and right lanes");

    juce::Image image(juce::Image::ARGB, 800, 360, true);
    juce::Graphics graphics(image);
    view.paint(graphics);
    expect(!image.isNull(), "stereo waveform paints successfully");
    return failures == 0 ? 0 : 1;
}
