#pragma once

#include "dozi/core/MasteringDecisionEngine.h"

#include <string>
#include <string_view>
#include <vector>

namespace dozi::core {

struct MasteringStylePreset {
    std::string id;
    std::string name;
    std::string description;
    double compressorRatio=1.5,compressorAttackMs=30,compressorReleaseMs=150,compressorThresholdOffsetDb=6;
    bool multiband=false;double multibandRatio=1.5,multibandMaximumReductionDb=2;
    bool saturation=false;double saturationDriveDb=2,saturationMix=0.2,saturationMode=0;
    bool deEsser=false;double deEsserMaximumReductionDb=3;
    double widthFactor=1,monoBassCrossoverHz=120;
    bool capTargetBoost=false;double maximumTargetBoostDb=12,maximumTruePeakCeilingDbtp=-0.1;
};

[[nodiscard]] const std::vector<MasteringStylePreset>& factoryMasteringStyles();
[[nodiscard]] const MasteringStylePreset* masteringStyle(std::string_view id);
void applyMasteringStyle(MasteringPlan&,const MasteringAnalysisResult&,const MasteringStylePreset&);

} // namespace dozi::core
