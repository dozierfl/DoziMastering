#pragma once

#include "dozi/core/MasteringAnalyzer.h"

#include <optional>
#include <string>
#include <vector>

namespace dozi::core {

enum class DiagnosticBasis { objectiveMeasurement, referenceComparison, genreProfile };

struct MasteringDiagnostic {
    std::string id;
    std::string descriptor;
    std::string family;
    std::string hypothesis;
    std::string evidence;
    std::string recommendation;
    DiagnosticBasis basis = DiagnosticBasis::objectiveMeasurement;
    double strength = 0.0;
    double lowerHz = 0.0;
    double upperHz = 0.0;
    bool preferDynamicProcessing = false;
    bool returnToMixRecommended = false;
};

struct MasteringDiagnosticConfig {
    double referenceBandDeviationDb = 2.5;
    double resonanceExcessDb = 4.0;
    double lowCrestFactorDb = 7.0;
    double highCrestFactorDb = 18.0;
    double unsafeCorrelation = 0.10;
};

class MasteringDiagnosticEngine final {
public:
    [[nodiscard]] std::vector<MasteringDiagnostic> diagnose(
        const MasteringAnalysisResult& source,
        const std::optional<MasteringAnalysisResult>& reference = {},
        const MasteringDiagnosticConfig& config = {}) const;
};

} // namespace dozi::core
