#include "dozi/core/MasteringHealth.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
namespace dozi::core { namespace {
double clampScore(double v) { return std::clamp(v, 0.0, 100.0); }
CategoryScore one(std::string name, std::vector<ScoreComponent> c) {
    double sum = 0; for (const auto& x : c) sum += x.contribution;
    return {std::move(name), clampScore(sum / std::max<std::size_t>(1, c.size())), std::move(c)};
}
}
MasteringHealthScore MasteringHealthScorer::score(const MasteringAnalysisResult& a, const HealthScoreConfig& c) const {
    if (a.bands.size() != c.referenceBandRelativeDb.size() || c.tonalToleranceDb <= 0
        || c.minimumCrestDb > c.maximumCrestDb) throw std::invalid_argument("Invalid health-score input");
    const auto measuredMax = std::ranges::max(a.bands, {}, &SpectralBandMeasurement::energyDb).energyDb;
    const auto referenceMax = *std::ranges::max_element(c.referenceBandRelativeDb);
    double squared = 0; for (std::size_t i=0;i<a.bands.size();++i) {
        const auto d=(a.bands[i].energyDb-measuredMax)-(c.referenceBandRelativeDb[i]-referenceMax); squared+=d*d;
    }
    const auto deviation=std::sqrt(squared/a.bands.size());
    auto tonal=one("Tonal Balance", {{"band-curve RMS deviation","100 * (1 - deviation / tolerance)",deviation,clampScore(100*(1-deviation/c.tonalToleranceDb))}});
    const auto crestDistance=a.crestFactorDb<c.minimumCrestDb?c.minimumCrestDb-a.crestFactorDb:a.crestFactorDb>c.maximumCrestDb?a.crestFactorDb-c.maximumCrestDb:0.0;
    auto dynamics=one("Dynamics", {{"crest factor","100 - 12.5 * distance outside configured range",a.crestFactorDb,clampScore(100-12.5*crestDistance)}});
    auto stereo=one("Stereo Image", {
        {"phase correlation","100 when >= minimum; linear penalty below",a.phaseCorrelation,clampScore(100-100*std::max(0.0,c.minimumCorrelation-a.phaseCorrelation))},
        {"mono compatibility loss","100 when >= limit; 20 points per dB below",a.monoCompatibilityLossDb,clampScore(100-20*std::max(0.0,c.maximumMonoLossDb-a.monoCompatibilityLossDb))}});
    auto loudness=one("Loudness/Streaming Readiness", {
        {"integrated loudness","100 - 10 * absolute target error",a.integratedLufs,clampScore(100-10*std::abs(a.integratedLufs-c.targetLufs))},
        {"true peak","100 when <= ceiling; 25 points per dB above",a.truePeakDbtp,clampScore(100-25*std::max(0.0,a.truePeakDbtp-c.truePeakCeilingDbtp))}});
    MasteringHealthScore result{{tonal,dynamics,stereo,loudness},0};
    for(const auto& category:result.categories) result.overall+=category.score;
    result.overall/=result.categories.size(); return result;
}}
