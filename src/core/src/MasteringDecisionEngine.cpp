#include "dozi/core/MasteringDecisionEngine.h"

#include <algorithm>
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

    MasteringModulePlan compressor {MasteringModuleType::broadbandCompressor, true, {}, {}};
    if (analysis.crestFactorDb >= config.compressionCrestThresholdDb) {
        compressor.bypassed = false;
        compressor.parameters = {{"thresholdDbfs", std::min(-6.0, analysis.rmsDbfs + 6.0)},
            {"ratio", config.compressorRatio}, {"attackMs", config.compressorAttackMs},
            {"releaseMs", config.compressorReleaseMs}, {"makeupGainDb", 0.0}};
        compressor.reasons.push_back(trace("crest factor", analysis.crestFactorDb, "dB",
            "COMPRESS_HIGH_CREST", "crest factor >= configured compression threshold",
            "Enable low-ratio broadband compression with zero automatic makeup gain"));
    }
    plan.modules.push_back(std::move(compressor));

    MasteringModulePlan stereo {MasteringModuleType::stereoWidthMonoBass, false,
        {{"widthFactor", 1.0}, {"monoBassCrossoverHz", config.monoBassCrossoverHz}}, {}};
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
            {"lookaheadMs", 5.0}, {"releaseMs", config.limiterReleaseMs}}, {}};
    limiter.reasons.push_back(trace("integrated loudness", analysis.integratedLufs, "LUFS",
        "TARGET_LOUDNESS_WITH_LOOKAHEAD_LIMITING", "gain = min(target-LUFS, maximum input gain)",
        "Apply " + std::to_string(inputGain) + " dB before linked look-ahead true-peak limiting"));
    limiter.reasons.push_back(trace("true peak", analysis.truePeakDbtp, "dBTP", "TRUE_PEAK_LIMIT",
        "linked look-ahead gain reduction plus verified safety trim", "Preserve the configured true-peak ceiling"));
    plan.modules.push_back(std::move(limiter));
    return plan;
}

} // namespace dozi::core
