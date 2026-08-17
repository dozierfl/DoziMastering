#pragma once

#include "dozi/core/MasteringDecisionEngine.h"

#include <optional>
#include <string>
#include <vector>

namespace dozi::core {

struct MasteringTargetPreset {
    std::string id;
    std::string displayName;
    double integratedLufs = -14.0;
    double truePeakCeilingDbtp = -1.0;
    std::string sourceUrl;
    std::string sourceNote;
};

[[nodiscard]] const std::vector<MasteringTargetPreset>& verifiedMasteringTargets();
[[nodiscard]] std::optional<MasteringTargetPreset> masteringTarget(std::string_view id);
[[nodiscard]] MasteringDecisionConfig decisionConfig(const MasteringTargetPreset&);

} // namespace dozi::core
