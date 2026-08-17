#pragma once

#include "dozi/core/CancellationToken.h"
#include "dozi/core/Error.h"

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dozi::core {

struct SpectralBandMeasurement {
    std::string name;
    double lowerHz = 0.0;
    double upperHz = 0.0;
    double energyDb = -120.0;
};

struct SpectralPeakMeasurement {
    double frequencyHz = 0.0;
    double excessDb = 0.0;
};

struct MasteringAnalysisResult {
    std::filesystem::path path;
    int sampleRateHz = 0;
    int channelCount = 0;
    int sourceBitDepth = 0;
    std::uint64_t frameCount = 0;
    double durationSeconds = 0.0;
    double integratedLufs = -120.0;
    double maximumShortTermLufs = -120.0;
    double maximumMomentaryLufs = -120.0;
    double samplePeakDbfs = -120.0;
    double truePeakDbtp = -120.0;
    double rmsDbfs = -120.0;
    double crestFactorDb = 0.0;
    double dcOffset = 0.0;
    double noiseFloorDbfs = -120.0;
    std::uint64_t clippedSampleCount = 0;
    bool interSamplePeakDetected = false;
    double phaseCorrelation = 0.0;
    double midSideEnergyRatioDb = 0.0;
    double monoCompatibilityLossDb = 0.0;
    double spectralCentroidHz = 0.0;
    std::vector<SpectralBandMeasurement> bands;
    std::vector<SpectralPeakMeasurement> resonances;
};

struct MasteringAnalysisOutcome {
    std::optional<MasteringAnalysisResult> result;
    std::optional<Error> error;
};

class MasteringAnalyzer final {
public:
    [[nodiscard]] MasteringAnalysisOutcome analyse(const std::filesystem::path&,
        const CancellationToken& cancellation = {}) const;
};

} // namespace dozi::core
