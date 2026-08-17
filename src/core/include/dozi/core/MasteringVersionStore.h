#pragma once

#include "dozi/core/MasteringDecisionEngine.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dozi::core {

struct MasteringVersionDifference {
    std::string path;
    std::optional<double> leftValue;
    std::optional<double> rightValue;
    std::string leftText;
    std::string rightText;
};

struct MasteringVersionOutcome {
    std::optional<MasteringPlan> plan;
    std::string error;
};

class MasteringVersionStore final {
public:
    [[nodiscard]] std::string save(const std::filesystem::path&, const MasteringPlan&) const;
    [[nodiscard]] MasteringVersionOutcome restore(const std::filesystem::path&) const;
    [[nodiscard]] std::string serialize(const MasteringPlan&) const;
    [[nodiscard]] MasteringVersionOutcome deserialize(const std::string&) const;
    [[nodiscard]] std::vector<MasteringVersionDifference> compare(
        const MasteringPlan&, const MasteringPlan&) const;
};

} // namespace dozi::core
