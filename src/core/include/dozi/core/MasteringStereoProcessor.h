#pragma once
#include <vector>
namespace dozi::core {
struct StereoAudioBuffer { int sampleRate = 0; std::vector<double> left, right; };
struct StereoProcessorSettings {
    bool bypassed = false;
    double widthFactor = 1.0;
    double monoBassCrossoverHz = 120.0;
    double sideTrimDb = 0.0;
    double balance = 0.0;
    bool monoCheck = false;
    bool swapLeftRight = false;
};
class MasteringStereoProcessor final {
public:
    void process(StereoAudioBuffer&, const StereoProcessorSettings&) const;
};
}
