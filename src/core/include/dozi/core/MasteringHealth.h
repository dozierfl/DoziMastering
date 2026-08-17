#pragma once
#include "dozi/core/MasteringAnalyzer.h"
#include <string>
#include <vector>
namespace dozi::core {
struct ScoreComponent { std::string measurement, formula; double value = 0.0, contribution = 0.0; };
struct CategoryScore { std::string name; double score = 0.0; std::vector<ScoreComponent> components; };
struct MasteringHealthScore { std::vector<CategoryScore> categories; double overall = 0.0; };
struct HealthScoreConfig {
    double targetLufs = -14.0, truePeakCeilingDbtp = -1.0;
    double minimumCrestDb = 8.0, maximumCrestDb = 14.0;
    double minimumCorrelation = 0.0, maximumMonoLossDb = -3.0;
    std::vector<double> referenceBandRelativeDb { -8, -3, -2, 0, -2, -6, -12 };
    double tonalToleranceDb = 6.0;
};
class MasteringHealthScorer final { public: [[nodiscard]] MasteringHealthScore score(
    const MasteringAnalysisResult&, const HealthScoreConfig& = {}) const; };
}
