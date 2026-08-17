#pragma once
#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringHealth.h"
namespace dozi::core {
struct MasteringRenderResult { std::filesystem::path output; MasteringAnalysisResult before,after; MasteringHealthScore beforeHealth,afterHealth; double limiterMaximumGainReductionDb=0, limiterSafetyTrimDb=0, limiterInputGainDb=0; };
class MasteringRenderer final { public: [[nodiscard]] MasteringRenderResult render(const std::filesystem::path&,const std::filesystem::path&,const MasteringPlan&,const CancellationToken& cancellation = {}) const; };
}
