#pragma once

#include "dozi/core/MasteringAnalyzer.h"

#include <map>
#include <string>
#include <vector>
#include <utility>

namespace dozi::core {

enum class MasteringModuleType {
    linearPhaseEq,
    dynamicEq,
    deEsser,
    broadbandCompressor,
    multibandCompressor,
    saturation,
    stereoWidthMonoBass,
    truePeakLimiter
};

struct MasteringDecisionTrace {
    std::string measurement;
    double measuredValue = 0.0;
    std::string unit;
    std::string ruleId;
    std::string ruleExpression;
    std::string action;
};

struct MasteringModulePlan {
    MasteringModulePlan() = default;
    MasteringModulePlan(MasteringModuleType moduleType,bool isBypassed,
        std::map<std::string,double> values,std::vector<MasteringDecisionTrace> decisionReasons)
        : type(moduleType),bypassed(isBypassed),parameters(std::move(values)),reasons(std::move(decisionReasons)) {}
    MasteringModuleType type;
    bool bypassed = false;
    std::map<std::string, double> parameters;
    std::vector<MasteringDecisionTrace> reasons;
    std::map<std::string, double> recommendedParameters;
    std::map<std::string, double> styleParameters;
};

struct MasteringPlan {
    int schemaVersion = 2;
    std::string targetId;
    std::string styleId = "balanced-streaming";
    double targetIntegratedLufs = -14.0;
    double truePeakCeilingDbtp = -1.0;
    std::vector<MasteringModulePlan> modules;
};

struct MasteringDecisionConfig {
    std::string targetId = "streaming-normal";
    double targetIntegratedLufs = -14.0;
    double truePeakCeilingDbtp = -1.0;
    double resonanceThresholdDb = 4.0;
    double maximumCorrectiveCutDb = 3.0;
    double correctiveQ = 4.0;
    double compressionCrestThresholdDb = 12.0;
    double compressorRatio = 1.5;
    double compressorAttackMs = 30.0;
    double compressorReleaseMs = 150.0;
    double unsafeCorrelationThreshold = 0.10;
    double unsafeMonoLossThresholdDb = -3.0;
    double safeWidthFactor = 0.90;
    double monoBassCrossoverHz = 120.0;
    double maximumLimiterInputGainDb = 12.0;
    double limiterReleaseMs = 100.0;
};

class MasteringDecisionEngine final {
public:
    [[nodiscard]] MasteringPlan createPlan(const MasteringAnalysisResult&,
        const MasteringDecisionConfig& = {}) const;
};

} // namespace dozi::core
