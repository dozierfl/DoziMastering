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

    view.setWaveform({envelope(1.0f), envelope(-1.0f)}, 180.0);
    expect(view.displayedChannelCount() == 2, "stereo input displays separate left and right lanes");

    juce::Image image(juce::Image::ARGB, 800, 360, true);
    juce::Graphics graphics(image);
    view.paint(graphics);
    expect(!image.isNull(), "stereo waveform paints successfully");
    return failures == 0 ? 0 : 1;
}
