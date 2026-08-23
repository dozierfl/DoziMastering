#pragma once

#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringRenderer.h"
#include "dozi/core/MasteringDsp.h"
#include "dozi/core/MasteringStereoProcessor.h"
#include <vector>
#include <optional>

namespace dozi::core {
class MasteringProcessorChain final {
public:
    void setPlan(MasteringPlan);
    void reset();
    [[nodiscard]] const MasteringPlan& plan() const noexcept { return currentPlan; }
    [[nodiscard]] std::vector<ProcessorTelemetry> process(StereoAudioBuffer&, bool auditionMode = false,
        ProcessorTelemetryCallback = {}, std::optional<MasteringModuleType> telemetryFilter = {});
private:
    MasteringPlan currentPlan;
    LinearPhaseEq linearPhaseEq;
    BroadbandCompressor broadbandCompressor;
    DynamicEqualizer dynamicEqualizer;
    DeEsser deEsser;
    MultibandCompressor multibandCompressor;
    HarmonicSaturator harmonicSaturator;
    TruePeakLimiter truePeakLimiter;
    MasteringStereoProcessor stereoProcessor;
};
}
