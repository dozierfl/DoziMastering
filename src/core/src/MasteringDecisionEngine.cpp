#include "dozi/core/MasteringDecisionEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace dozi::core {
namespace {

MasteringDecisionTrace trace(std::string measurement, double value, std::string unit,
    std::string rule, std::string expression, std::string action)
{
    return {std::move(measurement), value, std::move(unit), std::move(rule),
        std::move(expression), std::move(action)};
}

bool finiteAnalysis(const MasteringAnalysisResult& value)
{
    return std::isfinite(value.integratedLufs) && std::isfinite(value.truePeakDbtp)
        && std::isfinite(value.crestFactorDb) && std::isfinite(value.phaseCorrelation)
        && std::isfinite(value.monoCompatibilityLossDb);
}

} // namespace

MasteringPlan MasteringDecisionEngine::createPlan(
    const MasteringAnalysisResult& analysis, const MasteringDecisionConfig& config) const
{
    if (!finiteAnalysis(analysis) || !std::isfinite(config.targetIntegratedLufs)
        || !std::isfinite(config.truePeakCeilingDbtp) || config.truePeakCeilingDbtp > 0.0
        || config.resonanceThresholdDb < 0.0 || config.maximumCorrectiveCutDb < 0.0
        || config.correctiveQ <= 0.0 || config.compressorRatio < 1.0 || config.compressorAttackMs <= 0.0
        || config.compressorReleaseMs <= 0.0 || config.safeWidthFactor < 0.0 || config.safeWidthFactor > 1.0
        || config.monoBassCrossoverHz <= 0.0 || config.maximumLimiterInputGainDb < 0.0
        || config.limiterReleaseMs <= 0.0)
        throw std::invalid_argument("Invalid mastering analysis or decision configuration");

    MasteringPlan plan;
    plan.targetId = config.targetId;
    plan.targetIntegratedLufs = config.targetIntegratedLufs;
    plan.truePeakCeilingDbtp = config.truePeakCeilingDbtp;

    MasteringModulePlan equalizer {MasteringModuleType::linearPhaseEq, false, {}, {}};
    int band = 0;
    for (const auto& peak : analysis.resonances) {
        if (peak.excessDb < config.resonanceThresholdDb) continue;
        const auto cut = -std::min(config.maximumCorrectiveCutDb,
            (peak.excessDb - config.resonanceThresholdDb) * 0.5);
        const auto prefix = "band" + std::to_string(++band) + ".";
        equalizer.parameters[prefix + "frequencyHz"] = peak.frequencyHz;
        equalizer.parameters[prefix + "gainDb"] = cut;
        equalizer.parameters[prefix + "q"] = config.correctiveQ;
        equalizer.reasons.push_back(trace("spectral resonance excess", peak.excessDb, "dB",
            "EQ_RESONANCE_CUT", "excess >= configured resonance threshold",
            "Apply " + std::to_string(cut) + " dB corrective cut at " + std::to_string(peak.frequencyHz) + " Hz"));
    }
    equalizer.bypassed = equalizer.parameters.empty();
    plan.modules.push_back(std::move(equalizer));

    MasteringModulePlan dynamicEqualizer {MasteringModuleType::dynamicEq, true, {}, {}};
    int dynamicBand = 0;
    for (const auto& peak : analysis.resonances) {
        if (peak.excessDb < config.resonanceThresholdDb) continue;
        const auto prefix = "band" + std::to_string(++dynamicBand) + ".";
        dynamicEqualizer.parameters[prefix + "frequencyHz"] = peak.frequencyHz;
        dynamicEqualizer.parameters[prefix + "gainDb"] = -std::min(2.0, peak.excessDb * 0.25);
        dynamicEqualizer.parameters[prefix + "q"] = config.correctiveQ;
        dynamicEqualizer.parameters[prefix + "thresholdDbfs"] = std::clamp(analysis.rmsDbfs - 6.0, -42.0, -18.0);
        dynamicEqualizer.parameters[prefix + "attackMs"] = 10.0;
        dynamicEqualizer.parameters[prefix + "releaseMs"] = 120.0;
        dynamicEqualizer.parameters[prefix + "enabled"] = 1.0;
        dynamicEqualizer.parameters[prefix + "listen"] = 0.0;
    }
    dynamicEqualizer.bypassed = dynamicBand == 0;
    static constexpr std::array<double, 4> defaultFrequencies {120.0, 500.0, 2500.0, 8000.0};
    while (dynamicBand < 4) {
        const auto prefix = "band" + std::to_string(++dynamicBand) + ".";
        dynamicEqualizer.parameters[prefix + "frequencyHz"] = defaultFrequencies[static_cast<std::size_t>(dynamicBand - 1)];
        dynamicEqualizer.parameters[prefix + "gainDb"] = 0.0;
        dynamicEqualizer.parameters[prefix + "q"] = 1.2;
        dynamicEqualizer.parameters[prefix + "thresholdDbfs"] = -26.0;
        dynamicEqualizer.parameters[prefix + "attackMs"] = 10.0;
        dynamicEqualizer.parameters[prefix + "releaseMs"] = 120.0;
        dynamicEqualizer.parameters[prefix + "enabled"] = 0.0;
        dynamicEqualizer.parameters[prefix + "listen"] = 0.0;
    }
    plan.modules.push_back(std::move(dynamicEqualizer));

    MasteringModulePlan deEsser {MasteringModuleType::deEsser, true,
        {{"frequencyHz", 6500.0}, {"q", 1.0},
            {"thresholdDbfs", std::clamp(analysis.rmsDbfs - 8.0, -42.0, -20.0)},
            {"ratio", 3.0}, {"maximumReductionDb", 3.0}, {"attackMs", 2.0},
            {"releaseMs", 80.0}, {"listen", 0.0}}, {}};
    plan.modules.push_back(std::move(deEsser));

    MasteringModulePlan compressor {MasteringModuleType::broadbandCompressor, true, {}, {}};
    if (analysis.crestFactorDb >= config.compressionCrestThresholdDb) {
        compressor.bypassed = false;
        compressor.parameters = {{"thresholdDbfs", std::min(-6.0, analysis.rmsDbfs + 6.0)},
            {"ratio", config.compressorRatio}, {"attackMs", config.compressorAttackMs},
            {"releaseMs", config.compressorReleaseMs}, {"makeupGainDb", 0.0},
            {"sidechainHpfHz", 80.0}, {"automaticRelease", 0.0}};
        compressor.reasons.push_back(trace("crest factor", analysis.crestFactorDb, "dB",
            "COMPRESS_HIGH_CREST", "crest factor >= configured compression threshold",
            "Enable low-ratio broadband compression with zero automatic makeup gain"));
    }
    plan.modules.push_back(std::move(compressor));

    MasteringModulePlan multiband {MasteringModuleType::multibandCompressor, true,
        {{"lowCrossoverHz", 120.0}, {"midCrossoverHz", 700.0}, {"highCrossoverHz", 4000.0},
            {"ratio", 1.4}, {"maximumGainReductionDb", 1.5}, {"low.thresholdDbfs", -18.0},
            {"lowMid.thresholdDbfs", -18.0}, {"highMid.thresholdDbfs", -19.0},
            {"high.thresholdDbfs", -20.0}, {"low.solo", 0.0}, {"lowMid.solo", 0.0},
            {"highMid.solo", 0.0}, {"high.solo", 0.0}}, {}};
    plan.modules.push_back(std::move(multiband));

    MasteringModulePlan saturation {MasteringModuleType::saturation, true,
        {{"character", 1.0}, {"driveDb", 0.0}, {"mix", 0.0}, {"outputTrimDb", 0.0},
            {"oversamplingFactor", 4.0}, {"automaticLevelCompensation", 1.0},
            {"maximumCompensationDb", 3.0}, {"outputCeilingDbfs", -0.1}}, {}};
    plan.modules.push_back(std::move(saturation));

    MasteringModulePlan stereo {MasteringModuleType::stereoWidthMonoBass, false,
        {{"widthFactor", 1.0}, {"monoBassCrossoverHz", config.monoBassCrossoverHz},
            {"sideTrimDb", 0.0}, {"balance", 0.0}, {"monoCheck", 0.0}, {"swapLeftRight", 0.0}}, {}};
    if (analysis.phaseCorrelation < config.unsafeCorrelationThreshold
        || analysis.monoCompatibilityLossDb < config.unsafeMonoLossThresholdDb) {
        stereo.parameters["widthFactor"] = config.safeWidthFactor;
        const bool correlationTriggered = analysis.phaseCorrelation < config.unsafeCorrelationThreshold;
        stereo.reasons.push_back(trace(correlationTriggered ? "phase correlation" : "mono compatibility loss",
            correlationTriggered ? analysis.phaseCorrelation : analysis.monoCompatibilityLossDb,
            correlationTriggered ? "coefficient" : "dB", "REDUCE_UNSAFE_WIDTH",
            "correlation or mono loss exceeds configured safety boundary",
            "Reduce side level and mono frequencies below the configured crossover"));
    } else {
        stereo.reasons.push_back(trace("phase correlation", analysis.phaseCorrelation, "coefficient",
            "PRESERVE_SAFE_WIDTH", "stereo measurements are within configured safety boundaries",
            "Preserve width and mono only frequencies below the configured crossover"));
    }
    plan.modules.push_back(std::move(stereo));

    const auto loudnessGain = config.targetIntegratedLufs - analysis.integratedLufs;
    const auto inputGain = std::min(loudnessGain, config.maximumLimiterInputGainDb);
    MasteringModulePlan limiter {MasteringModuleType::truePeakLimiter, false,
        {{"inputGainDb", inputGain}, {"ceilingDbtp", config.truePeakCeilingDbtp},
            {"lookaheadMs", 5.0}, {"releaseMs", config.limiterReleaseMs},
            {"linked", 1.0}, {"truePeakEnabled", 1.0}}, {}};
    limiter.reasons.push_back(trace("integrated loudness", analysis.integratedLufs, "LUFS",
        "TARGET_LOUDNESS_WITH_LOOKAHEAD_LIMITING", "gain = min(target-LUFS, maximum input gain)",
        "Apply " + std::to_string(inputGain) + " dB before linked look-ahead true-peak limiting"));
    limiter.reasons.push_back(trace("true peak", analysis.truePeakDbtp, "dBTP", "TRUE_PEAK_LIMIT",
        "linked look-ahead gain reduction plus verified safety trim", "Preserve the configured true-peak ceiling"));
    plan.modules.push_back(std::move(limiter));
    for(auto&module:plan.modules){module.recommendedParameters=module.parameters;module.styleParameters=module.parameters;}
    return plan;
}

} // namespace dozi::core
