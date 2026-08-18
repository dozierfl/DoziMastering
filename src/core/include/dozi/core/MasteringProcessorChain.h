#pragma once

#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringRenderer.h"
#include "dozi/core/MasteringDsp.h"
#include "dozi/core/MasteringStereoProcessor.h"
#include <vector>

namespace dozi::core {
class MasteringProcessorChain final {
public:
    void setPlan(MasteringPlan);
    void reset();
    [[nodiscard]] const MasteringPlan& plan() const noexcept { return currentPlan; }
    [[nodiscard]] std::vector<ProcessorTelemetry> process(StereoAudioBuffer&, bool auditionMode = false,
        ProcessorTelemetryCallback = {});
private:
    MasteringPlan currentPlan;
    BroadbandCompressor broadbandCompressor;
    MasteringStereoProcessor stereoProcessor;
};
}
