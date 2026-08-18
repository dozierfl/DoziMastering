#pragma once
#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringHealth.h"
#include <array>
#include <functional>
namespace dozi::core {
struct ProcessorTelemetry {
    MasteringModuleType type=MasteringModuleType::linearPhaseEq;
    double inputPeakDbfs=-120,outputPeakDbfs=-120,inputRmsDbfs=-120,outputRmsDbfs=-120;
    double maximumGainReductionDb=0,phaseCorrelation=1;
    std::array<double,24> spectrumDbfs{};
    std::vector<std::array<double,2>> vectorscope;
    std::size_t peakEventCount=0;
};
using ProcessorTelemetryCallback=std::function<void(const ProcessorTelemetry&)>;
struct MasteringRenderResult { std::filesystem::path output; MasteringAnalysisResult before,after; MasteringHealthScore beforeHealth,afterHealth; double limiterMaximumGainReductionDb=0, limiterSafetyTrimDb=0, limiterInputGainDb=0; std::vector<ProcessorTelemetry> telemetry; };
class MasteringRenderer final { public: [[nodiscard]] MasteringRenderResult render(const std::filesystem::path&,const std::filesystem::path&,const MasteringPlan&,const CancellationToken& cancellation = {},bool auditionMode = false,ProcessorTelemetryCallback = {}) const; };
}
