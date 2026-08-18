#include "dozi/core/MasteringReference.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace dozi::core {
namespace {

double finiteDifference(double source, double reference) {
    return std::isfinite(source) && std::isfinite(reference) ? source - reference : 0.0;
}

double bandMean(const std::vector<SpectralBandMeasurement>& bands) {
    if (bands.empty()) return 0.0;
    return std::accumulate(bands.begin(), bands.end(), 0.0,
        [](double sum, const auto& band) { return sum + band.energyDb; })
        / static_cast<double>(bands.size());
}

double geometricCentre(double lowerHz, double upperHz) {
    return std::sqrt(std::max(1.0, lowerHz) * std::max(1.0, upperHz));
}
const MasteringModulePlan* module(const std::vector<MasteringModulePlan>& modules,MasteringModuleType type){
    const auto found=std::find_if(modules.begin(),modules.end(),[type](const auto& value){return value.type==type;});
    return found==modules.end()?nullptr:&*found;
}

} // namespace

MasteringReferenceComparison MasteringReferenceMatcher::compare(
    const MasteringAnalysisResult& source,
    const MasteringAnalysisResult& reference) const {
    MasteringReferenceComparison result;
    result.integratedLoudnessDifferenceLu = finiteDifference(source.integratedLufs, reference.integratedLufs);
    result.truePeakDifferenceDb = finiteDifference(source.truePeakDbtp, reference.truePeakDbtp);
    result.crestFactorDifferenceDb = finiteDifference(source.crestFactorDb, reference.crestFactorDb);
    result.phaseCorrelationDifference = finiteDifference(source.phaseCorrelation, reference.phaseCorrelation);
    result.monoCompatibilityDifferenceDb = finiteDifference(
        source.monoCompatibilityLossDb, reference.monoCompatibilityLossDb);
    result.spectralCentroidDifferenceHz = finiteDifference(
        source.spectralCentroidHz, reference.spectralCentroidHz);

    const auto sourceMean = bandMean(source.bands);
    const auto referenceMean = bandMean(reference.bands);
    std::map<std::string, SpectralBandMeasurement> referenceByName;
    for (const auto& band : reference.bands) referenceByName.emplace(band.name, band);
    for (const auto& sourceBand : source.bands) {
        const auto found = referenceByName.find(sourceBand.name);
        if (found == referenceByName.end()) continue;
        const auto sourceRelative = sourceBand.energyDb - sourceMean;
        const auto referenceRelative = found->second.energyDb - referenceMean;
        result.bands.push_back({sourceBand.name, sourceBand.lowerHz, sourceBand.upperHz,
            sourceRelative, referenceRelative, sourceRelative - referenceRelative});
    }
    return result;
}

ReferenceMatchPlan MasteringReferenceMatcher::createPlan(
    const MasteringAnalysisResult& source,
    const MasteringAnalysisResult& reference,
    const ReferenceMatchConfig& config) const {
    ReferenceMatchPlan result;
    result.comparison = compare(source, reference);

    MasteringModulePlan equalizer {MasteringModuleType::dynamicEq, true, {}, {}};
    int bandNumber = 1;
    for (const auto& band : result.comparison.bands) {
        if (std::abs(band.differenceDb) < config.minimumTonalDifferenceDb) continue;
        // Phase 2 reference matching is corrective: excess source energy may be
        // reduced, but a reference deficit never causes an automatic boost.
        if (band.differenceDb <= 0.0) continue;
        const auto adjustment = std::clamp(-band.differenceDb,
            -config.maximumEqAdjustmentDb, 0.0);
        const auto prefix = "band" + std::to_string(bandNumber++) + ".";
        equalizer.parameters[prefix + "frequencyHz"] = geometricCentre(band.lowerHz, band.upperHz);
        equalizer.parameters[prefix + "gainDb"] = adjustment;
        equalizer.parameters[prefix + "q"] = config.equalizerQ;
        equalizer.parameters[prefix + "thresholdDbfs"] = -26.0;
        equalizer.parameters[prefix + "attackMs"] = 10.0;
        equalizer.parameters[prefix + "releaseMs"] = 120.0;
        equalizer.parameters[prefix + "enabled"] = 1.0;
        equalizer.parameters[prefix + "listen"] = 0.0;
        equalizer.reasons.push_back({"relative band energy difference", band.differenceDb, "dB",
            "reference-tonal-difference", "abs(source-relative-reference-relative) >= threshold",
            "Propose bounded dynamic EQ adjustment for " + band.name});
    }
    equalizer.bypassed = equalizer.parameters.empty();
    result.proposedModules.push_back(std::move(equalizer));

    MasteringModulePlan compressor {MasteringModuleType::multibandCompressor, true, {}, {}};
    if (result.comparison.crestFactorDifferenceDb > config.minimumCrestDifferenceDb) {
        compressor.bypassed = false;
        compressor.parameters = {{"ratio", 1.5}, {"maximumGainReductionDb", 2.0}};
        compressor.reasons.push_back({"crest factor difference",
            result.comparison.crestFactorDifferenceDb, "dB", "reference-crest-difference",
            "source crest - reference crest > threshold",
            "Propose bounded multiband dynamics control"});
    }
    result.proposedModules.push_back(std::move(compressor));
    for(auto&module:result.proposedModules){module.recommendedParameters=module.parameters;module.styleParameters=module.parameters;}
    return result;
}

ReferenceMatchOutcome MasteringReferenceMatcher::analyseFiles(
    const std::filesystem::path& source,
    const std::filesystem::path& reference,
    const ReferenceMatchConfig& config,
    const CancellationToken& cancellation) const {
    MasteringAnalyzer analyzer;
    const auto sourceAnalysis = analyzer.analyse(source, cancellation);
    if (cancellation.isCancelled()) return {};
    if (!sourceAnalysis.result) return {{}, sourceAnalysis.error};
    const auto referenceAnalysis = analyzer.analyse(reference, cancellation);
    if (cancellation.isCancelled()) return {};
    if (!referenceAnalysis.result) return {{}, referenceAnalysis.error};
    if (sourceAnalysis.result->sampleRateHz != referenceAnalysis.result->sampleRateHz)
        return {{}, Error {ErrorCode::unsupportedWaveFormat,
            "Source and reference sample rates must match for deterministic comparison."}};
    if (sourceAnalysis.result->channelCount != 2 || referenceAnalysis.result->channelCount != 2)
        return {{}, Error {ErrorCode::unsupportedWaveFormat,
            "Reference matching requires stereo source and reference files."}};
    return {createPlan(*sourceAnalysis.result, *referenceAnalysis.result, config), {}};
}

MasteringPlan MasteringReferenceMatcher::createPhase2Plan(
    const MasteringAnalysisResult& source,
    const std::optional<MasteringAnalysisResult>& reference,
    const MasteringDecisionConfig& decisionConfig,
    const ReferenceMatchConfig& matchConfig) const {
    auto base=MasteringDecisionEngine{}.createPlan(source,decisionConfig);
    std::optional<ReferenceMatchPlan> match;
    if(reference)match=createPlan(source,*reference,matchConfig);
    std::vector<MasteringModulePlan> modules;
    modules.push_back(*module(base.modules,MasteringModuleType::linearPhaseEq));
    const auto* dynamic=match?module(match->proposedModules,MasteringModuleType::dynamicEq):nullptr;
    modules.push_back(dynamic?*dynamic:MasteringModulePlan{MasteringModuleType::dynamicEq,true,{},{}});
    modules.push_back({MasteringModuleType::deEsser,true,{{"frequencyHz",6500},{"q",1},{"thresholdDbfs",-30},{"ratio",3},{"maximumReductionDb",6},{"attackMs",2},{"releaseMs",80},{"listen",0}}, {}});
    modules.push_back(*module(base.modules,MasteringModuleType::broadbandCompressor));
    const auto* multiband=match?module(match->proposedModules,MasteringModuleType::multibandCompressor):nullptr;
    modules.push_back(multiband?*multiband:MasteringModulePlan{MasteringModuleType::multibandCompressor,true,{{"lowCrossoverHz",120},{"midCrossoverHz",700},{"highCrossoverHz",4000},{"ratio",1.5},{"maximumGainReductionDb",2},{"low.thresholdDbfs",-18},{"lowMid.thresholdDbfs",-18},{"highMid.thresholdDbfs",-19},{"high.thresholdDbfs",-20},{"low.solo",0},{"lowMid.solo",0},{"highMid.solo",0},{"high.solo",0}}, {}});
    modules.push_back({MasteringModuleType::saturation,true,{{"character",1},{"driveDb",2},{"mix",0.5},{"outputTrimDb",0},{"oversamplingFactor",4},{"automaticLevelCompensation",1},{"maximumCompensationDb",3},{"outputCeilingDbfs",-0.1}}, {}});
    modules.push_back(*module(base.modules,MasteringModuleType::stereoWidthMonoBass));
    modules.push_back(*module(base.modules,MasteringModuleType::truePeakLimiter));
    base.modules=std::move(modules);return base;
}

} // namespace dozi::core
