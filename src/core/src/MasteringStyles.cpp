#include "dozi/core/MasteringStyles.h"

#include <algorithm>

namespace dozi::core {
namespace {
MasteringModulePlan* module(MasteringPlan&plan,MasteringModuleType type){const auto found=std::ranges::find(plan.modules,type,&MasteringModulePlan::type);return found==plan.modules.end()?nullptr:&*found;}
}

const std::vector<MasteringStylePreset>& factoryMasteringStyles(){
    static const std::vector<MasteringStylePreset> styles{
        {"dynamic-acoustic","Dynamic Acoustic","Maximum openness for acoustic, jazz, orchestral, and delicate mixes.",1.2,60,240,8,false,1.2,1,false,0,0.0,0,false,2,1,80},
        {"balanced-streaming","Balanced Streaming","Transparent general-purpose polish with restrained density.",1.5,40,160,6,false,1.4,1.5,true,1,0.12,0,false,2,1,100},
        {"modern-balanced","Modern Balanced","More density and harmonic polish without an aggressive contour.",1.6,30,140,5,true,1.4,1.5,true,1.5,0.18,0,false,2.5,1,100},
        {"punchy-rock","Punchy Rock","Slower transient-preserving glue with controlled midband density.",1.7,55,120,5,true,1.5,2,true,1.5,0.18,0,false,2.5,1,100},
        {"modern-pop","Modern Pop","Forward, controlled, polished pop density with gentle de-essing.",1.8,25,100,4,true,1.6,2,true,2.5,0.25,1,true,3,1,110},
        {"loud-club","Loud Club","Dense dance-floor presentation with bounded multiband and saturation stages.",2.0,20,85,3,true,1.8,3,true,3.5,0.35,1,true,3,1,120},
        {"low-end-hip-hop","Low-End Hip-Hop","Preserves drum impact while stabilizing bass and mono-compatible sub energy.",1.6,45,130,5,true,1.5,2.5,true,2,0.2,0,false,2.5,1,90},
        {"warm-rnb-soul","Warm R&B / Soul","Smooth glue and restrained tape-style harmonic warmth.",1.4,45,180,6,false,1.4,1.5,true,2,0.22,0,false,2,1,90},
        {"podcast-spoken","Podcast / Spoken Word","Consistent centered speech with stronger level control and de-essing.",3.0,10,110,3,true,2,3,false,0,0,0,true,5,0,120},
        {"suno-wav-remaster","AI WAV Remaster","Conservative repair and streaming-safe remastering for already-loud or limited AI-generated WAV exports.",1.05,80,300,10,false,1.1,0.75,false,0,0,0,true,1.5,0.98,100,true,0.5,-1.0},
        {"remaster-clean","Remaster Clean","Corrective processing with almost no added dynamics or harmonic color.",1.1,80,300,9,false,1.1,1,false,0,0,0,false,1.5,1,70},
        {"remaster-vibe","Remaster Vibe","Gentle glue with subtle tape color while retaining the source character.",1.3,55,220,7,false,1.3,1.5,true,1.5,0.16,0,false,2,1,80}
    };return styles;
}

const MasteringStylePreset* masteringStyle(std::string_view id){const auto&styles=factoryMasteringStyles();const auto found=std::ranges::find(styles,id,&MasteringStylePreset::id);return found==styles.end()?nullptr:&*found;}

void applyMasteringStyle(MasteringPlan&plan,const MasteringAnalysisResult&analysis,const MasteringStylePreset&style){
    plan.styleId=style.id;
    if(style.capTargetBoost){const auto previousTarget=plan.targetIntegratedLufs,previousCeiling=plan.truePeakCeilingDbtp;plan.targetIntegratedLufs=std::min(previousTarget,analysis.integratedLufs+style.maximumTargetBoostDb);plan.truePeakCeilingDbtp=std::min(previousCeiling,style.maximumTruePeakCeilingDbtp);if(plan.targetIntegratedLufs!=previousTarget||plan.truePeakCeilingDbtp!=previousCeiling)plan.targetId="custom";}
    if(auto*value=module(plan,MasteringModuleType::broadbandCompressor)){value->bypassed=style.compressorRatio<=1.1;value->parameters={{"thresholdDbfs",std::clamp(analysis.rmsDbfs+style.compressorThresholdOffsetDb,-30.0,-6.0)},{"ratio",style.compressorRatio},{"attackMs",style.compressorAttackMs},{"releaseMs",style.compressorReleaseMs},{"makeupGainDb",0},{"sidechainHpfHz",80},{"automaticRelease",0}};}
    if(auto*value=module(plan,MasteringModuleType::multibandCompressor)){value->bypassed=!style.multiband;value->parameters={{"lowCrossoverHz",120},{"midCrossoverHz",700},{"highCrossoverHz",4000},{"ratio",style.multibandRatio},{"maximumGainReductionDb",style.multibandMaximumReductionDb},{"low.thresholdDbfs",-18},{"lowMid.thresholdDbfs",-18},{"highMid.thresholdDbfs",-19},{"high.thresholdDbfs",-20},{"low.solo",0},{"lowMid.solo",0},{"highMid.solo",0},{"high.solo",0}};}
    if(auto*value=module(plan,MasteringModuleType::saturation)){value->bypassed=!style.saturation;value->parameters={{"character",std::clamp(style.saturationMode+1.0,0.0,2.0)},{"driveDb",style.saturationDriveDb},{"mix",style.saturationMix},{"outputTrimDb",0},{"oversamplingFactor",4},{"automaticLevelCompensation",1},{"maximumCompensationDb",3},{"outputCeilingDbfs",-0.1}};}
    if(auto*value=module(plan,MasteringModuleType::deEsser)){value->bypassed=!style.deEsser;value->parameters={{"frequencyHz",6500},{"q",1},{"thresholdDbfs",std::clamp(analysis.rmsDbfs-8,-42.0,-20.0)},{"ratio",3},{"maximumReductionDb",style.deEsserMaximumReductionDb},{"attackMs",2},{"releaseMs",80},{"listen",0}};}
    if(auto*value=module(plan,MasteringModuleType::stereoWidthMonoBass)){value->bypassed=false;value->parameters["widthFactor"]=style.widthFactor;value->parameters["monoBassCrossoverHz"]=style.monoBassCrossoverHz;value->parameters.try_emplace("sideTrimDb",0);value->parameters.try_emplace("balance",0);value->parameters.try_emplace("monoCheck",0);value->parameters.try_emplace("swapLeftRight",0);}
    if(auto*value=module(plan,MasteringModuleType::truePeakLimiter)){value->parameters["inputGainDb"]=std::min(value->parameters["inputGainDb"],style.maximumTargetBoostDb);value->parameters["ceilingDbtp"]=std::min(value->parameters["ceilingDbtp"],style.maximumTruePeakCeilingDbtp);value->parameters.try_emplace("linked",1);value->parameters.try_emplace("truePeakEnabled",1);}
    for(auto&value:plan.modules){if(value.recommendedParameters.empty())value.recommendedParameters=value.parameters;value.styleParameters=value.parameters;}
}

} // namespace dozi::core
