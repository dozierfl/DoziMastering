#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

struct GlueCompressorSettings {
    float thresholdDbfs = -11.4f;
    float attackMs = 80.0f;
    float releaseMs = 300.0f;
    float ratio = 1.1f;
    float makeupGainDb = 0.0f;
    bool autoMakeup = false;
    bool autoRelease = false;
    bool sidechainHpf = false;
    bool moduleEnabled = true;
};
struct GlueCompressorLevels {
    std::array<float, 2> inputDbfs {{-100.0f, -100.0f}};
    std::array<float, 2> outputDbfs {{-100.0f, -100.0f}};
    std::array<float, 2> gainReductionDb {{0.0f, 0.0f}};
};

class GlueCompressorEngine final {
public:
    void prepare(double sampleRate, int maximumBlockSize, int channels);
    void reset();
    [[nodiscard]] GlueCompressorLevels process(juce::AudioBuffer<float>&, const GlueCompressorSettings&);
    [[nodiscard]] static float softKneeGainReduction(float inputDbfs, float thresholdDbfs, float ratio);
    [[nodiscard]] static float automaticMakeupDb(float thresholdDbfs, float ratio);

private:
    struct HpfState { float x1=0, x2=0, y1=0, y2=0; };
    [[nodiscard]] float detectorHpf(float sample, int channel);
    double rate = 44100.0;
    float detectorEnvelopeSquared = 0.0f;
    float reductionEnvelopeDb = 0.0f;
    float sustainedReductionDb = 0.0f;
    std::array<HpfState, 2> hpf;
};
