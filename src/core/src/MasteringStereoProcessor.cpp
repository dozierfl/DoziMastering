#include "dozi/core/MasteringStereoProcessor.h"
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace dozi::core {
void MasteringStereoProcessor::process(StereoAudioBuffer& audio, const StereoProcessorSettings& settings) const
{
    if (audio.sampleRate <= 0 || audio.left.size() != audio.right.size() || settings.widthFactor < 0.0
        || settings.widthFactor > 1.0 || settings.monoBassCrossoverHz <= 0.0
        || settings.monoBassCrossoverHz >= audio.sampleRate * 0.5) throw std::invalid_argument("Invalid stereo processing input");
    if (settings.bypassed) return;
    const auto coefficient = 1.0 - std::exp(-2.0 * std::numbers::pi * settings.monoBassCrossoverHz / audio.sampleRate);
    double lowSide = 0.0;
    for (std::size_t i = 0; i < audio.left.size(); ++i) {
        const auto mid = (audio.left[i] + audio.right[i]) * 0.5;
        const auto side = (audio.left[i] - audio.right[i]) * 0.5;
        lowSide += coefficient * (side - lowSide);
        const auto safeSide = (side - lowSide) * settings.widthFactor;
        audio.left[i] = mid + safeSide;
        audio.right[i] = mid - safeSide;
    }
}
}
