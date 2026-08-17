#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringDsp.h"
#include "dozi/core/MasteringReference.h"
#include "dozi/core/MasteringStyles.h"
#include "dozi/core/MasteringTargets.h"
#include "dozi/core/MasteringVersionStore.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <ranges>

namespace {
int failures=0;
void expect(bool condition,const char*message){if(!condition){std::cerr<<"FAIL: "<<message<<'\n';++failures;}}
}

int main(){
    dozi::core::MasteringAnalysisResult analysis;
    analysis.integratedLufs=-8.0;analysis.truePeakDbtp=-0.2;analysis.rmsDbfs=-9.5;
    analysis.crestFactorDb=6.0;analysis.phaseCorrelation=0.82;analysis.monoCompatibilityLossDb=-0.4;
    analysis.resonances={{80.0,4.0},{3200.0,3.0}};

    auto plan=dozi::core::MasteringReferenceMatcher{}.createPhase2Plan(analysis,{});
    const auto*suno=dozi::core::masteringStyle("suno-wav-remaster");
    expect(suno!=nullptr,"Suno WAV Remaster preset is available");
    if(suno)dozi::core::applyMasteringStyle(plan,analysis,*suno);
    expect(plan.styleId=="suno-wav-remaster","Suno preset is applied to the plan");
    expect(plan.truePeakCeilingDbtp<=-1.0,"Suno preset retains a safe true-peak ceiling");
    const auto compressor=std::ranges::find(plan.modules,dozi::core::MasteringModuleType::broadbandCompressor,&dozi::core::MasteringModulePlan::type);
    expect(compressor!=plan.modules.end()&&compressor->bypassed,"Suno preset avoids additional broadband compression");

    const auto&styles=dozi::core::factoryMasteringStyles();
    expect(styles.size()==12,"all factory mastering styles are available");
    expect(dozi::core::masteringTarget("spotify-loud").has_value(),"Spotify Loud delivery target is available");

    const auto serialized=dozi::core::MasteringVersionStore{}.serialize(plan);
    const auto restored=dozi::core::MasteringVersionStore{}.deserialize(serialized);
    expect(restored.plan&&restored.plan->styleId==plan.styleId,"mastering plans round-trip through version storage");

    constexpr int rate=48000;
    dozi::core::StereoAudioBuffer audio;audio.sampleRate=rate;audio.left.resize(rate);audio.right.resize(rate);
    for(int sample=0;sample<rate;++sample){const auto value=0.4*std::sin(2.0*std::numbers::pi*80.0*sample/rate);audio.left[static_cast<std::size_t>(sample)]=value;audio.right[static_cast<std::size_t>(sample)]=value;}
    const auto before=audio.left;
    dozi::core::LinearPhaseEq{}.process(audio,{false,{{80.0,-0.5,0.7}},513});
    double beforePower=0,afterPower=0;for(std::size_t i=0;i<audio.left.size();++i){beforePower+=before[i]*before[i];afterPower+=audio.left[i]*audio.left[i];}
    expect(afterPower<beforePower,"the conversational 80 Hz adjustment reduces low-frequency energy");

    if(failures==0)std::cout<<"All Dozi Mastering tests passed\n";
    return failures==0?0:1;
}
