#include "dozi/core/MasteringStereoProcessor.h"
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace dozi::core {
void MasteringStereoProcessor::process(StereoAudioBuffer& audio, const StereoProcessorSettings& settings) const
{
    if (audio.sampleRate <= 0 || audio.left.size() != audio.right.size() || settings.widthFactor < 0.0
        || settings.widthFactor > 1.5 || settings.balance < -1.0 || settings.balance > 1.0
        || settings.monoBassCrossoverHz <= 0.0
        || settings.monoBassCrossoverHz >= audio.sampleRate * 0.5) throw std::invalid_argument("Invalid stereo processing input");
    if (settings.bypassed) return;
    const auto coefficient = 1.0 - std::exp(-2.0 * std::numbers::pi * settings.monoBassCrossoverHz / audio.sampleRate);
    double lowSide = 0.0;
    for (std::size_t i = 0; i < audio.left.size(); ++i) {
        const auto mid = (audio.left[i] + audio.right[i]) * 0.5;
        const auto side = (audio.left[i] - audio.right[i]) * 0.5;
        lowSide += coefficient * (side - lowSide);
        const auto sideTrim = std::pow(10.0, settings.sideTrimDb / 20.0);
        const auto safeSide = (side - lowSide) * settings.widthFactor * sideTrim;
        auto left = mid + safeSide;
        auto right = mid - safeSide;
        const auto leftBalance = settings.balance > 0.0 ? 1.0 - settings.balance : 1.0;
        const auto rightBalance = settings.balance < 0.0 ? 1.0 + settings.balance : 1.0;
        left *= leftBalance; right *= rightBalance;
        if (settings.monoCheck) left = right = (left + right) * 0.5;
        if (settings.swapLeftRight) std::swap(left, right);
        audio.left[i] = left;
        audio.right[i] = right;
    }
}
}
