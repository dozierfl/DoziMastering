#pragma once
#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringHealth.h"
namespace dozi::core { class MasteringReport final { public:
static std::string text(const MasteringAnalysisResult&, const MasteringPlan&, const MasteringHealthScore&);
static std::string json(const MasteringAnalysisResult&, const MasteringPlan&, const MasteringHealthScore&);
}; }
