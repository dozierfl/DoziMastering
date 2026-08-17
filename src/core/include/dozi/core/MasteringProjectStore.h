#pragma once

#include "dozi/core/MasteringAnalyzer.h"
#include "dozi/core/MasteringDecisionEngine.h"

#include <filesystem>
#include <optional>
#include <string>

namespace dozi::core {

struct MasteringProject {
    std::filesystem::path sourcePath;
    std::filesystem::path referencePath;
    std::filesystem::path previewPath;
    std::filesystem::path exportPath;
    double playbackPositionSeconds = 0.0;
    bool playbackMastered = false;
    bool loudnessMatched = true;
    double selectionStartSeconds = 0.0;
    double selectionEndSeconds = 0.0;
    bool loopSelectionEnabled = false;
    std::optional<MasteringAnalysisResult> sourceAnalysis;
    std::optional<MasteringAnalysisResult> referenceAnalysis;
    std::optional<MasteringAnalysisResult> processedAnalysis;
    std::optional<MasteringPlan> plan;
};

struct MasteringProjectOutcome {
    std::optional<MasteringProject> project;
    std::string error;
};

class MasteringProjectStore final {
public:
    [[nodiscard]] std::string save(const std::filesystem::path&, const MasteringProject&) const;
    [[nodiscard]] MasteringProjectOutcome open(const std::filesystem::path&) const;
};

} // namespace dozi::core
