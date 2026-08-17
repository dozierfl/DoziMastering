#pragma once

#include "dozi/core/MasteringAnalyzer.h"
#include "dozi/core/MasteringDecisionEngine.h"

#include <string>
#include <optional>
#include <vector>

namespace dozi::core {

struct ReferenceBandDifference {
    std::string name;
    double lowerHz = 0.0;
    double upperHz = 0.0;
    double sourceRelativeDb = 0.0;
    double referenceRelativeDb = 0.0;
    double differenceDb = 0.0;
};

struct MasteringReferenceComparison {
    double integratedLoudnessDifferenceLu = 0.0;
    double truePeakDifferenceDb = 0.0;
    double crestFactorDifferenceDb = 0.0;
    double phaseCorrelationDifference = 0.0;
    double monoCompatibilityDifferenceDb = 0.0;
    double spectralCentroidDifferenceHz = 0.0;
    std::vector<ReferenceBandDifference> bands;
};

struct ReferenceMatchConfig {
    double minimumTonalDifferenceDb = 1.5;
    double maximumEqAdjustmentDb = 3.0;
    double equalizerQ = 0.7;
    double minimumCrestDifferenceDb = 2.0;
};

struct ReferenceMatchPlan {
    MasteringReferenceComparison comparison;
    std::vector<MasteringModulePlan> proposedModules;
};

struct ReferenceMatchOutcome {
    std::optional<ReferenceMatchPlan> result;
    std::optional<Error> error;
};

class MasteringReferenceMatcher final {
public:
    [[nodiscard]] MasteringReferenceComparison compare(
        const MasteringAnalysisResult& source,
        const MasteringAnalysisResult& reference) const;

    [[nodiscard]] ReferenceMatchPlan createPlan(
        const MasteringAnalysisResult& source,
        const MasteringAnalysisResult& reference,
        const ReferenceMatchConfig& config = {}) const;

    [[nodiscard]] ReferenceMatchOutcome analyseFiles(
        const std::filesystem::path& source,
        const std::filesystem::path& reference,
        const ReferenceMatchConfig& config = {},
        const CancellationToken& cancellation = {}) const;

    [[nodiscard]] MasteringPlan createPhase2Plan(
        const MasteringAnalysisResult& source,
        const std::optional<MasteringAnalysisResult>& reference,
        const MasteringDecisionConfig& decisionConfig = {},
        const ReferenceMatchConfig& matchConfig = {}) const;
};

} // namespace dozi::core
