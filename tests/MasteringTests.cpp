#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringDiagnostics.h"
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
    const auto*aiRemaster=dozi::core::masteringStyle("suno-wav-remaster");
    expect(aiRemaster!=nullptr,"AI WAV Remaster preset is available");
    if(aiRemaster)dozi::core::applyMasteringStyle(plan,analysis,*aiRemaster);
    expect(plan.styleId=="suno-wav-remaster","AI remaster preset is applied to the plan");
    expect(plan.truePeakCeilingDbtp<=-1.0,"AI remaster preset retains a safe true-peak ceiling");
    const auto compressor=std::ranges::find(plan.modules,dozi::core::MasteringModuleType::broadbandCompressor,&dozi::core::MasteringModulePlan::type);
    expect(compressor!=plan.modules.end()&&compressor->bypassed,"AI remaster preset avoids additional broadband compression");

    const auto&styles=dozi::core::factoryMasteringStyles();
    expect(styles.size()==12,"all factory mastering styles are available");
    expect(dozi::core::masteringTarget("spotify-loud").has_value(),"Spotify Loud delivery target is available");

    dozi::core::MasteringAnalysisResult diagnosticSource,diagnosticReference;
    diagnosticSource.crestFactorDb=11.0;diagnosticSource.phaseCorrelation=0.8;
    diagnosticReference.crestFactorDb=11.0;diagnosticReference.phaseCorrelation=0.8;
    const std::array names{"sub","low","low-mid","mid","high-mid","high","air"};
    for(const auto*name:names){diagnosticSource.bands.push_back({name,20,200,-30});diagnosticReference.bands.push_back({name,20,200,-30});}
    diagnosticSource.bands[2].energyDb=-24.0;
    const auto diagnoses=dozi::core::MasteringDiagnosticEngine{}.diagnose(diagnosticSource,diagnosticReference);
    expect(std::ranges::any_of(diagnoses,[](const auto&finding){return finding.descriptor=="Muddy / cloudy"&&finding.basis==dozi::core::DiagnosticBasis::referenceComparison;}),"reference-relative low-mid excess produces an evidence-backed muddiness hypothesis");
    expect(std::ranges::none_of(diagnoses,[](const auto&finding){return finding.basis==dozi::core::DiagnosticBasis::genreProfile;}),"diagnostics do not invent a genre profile");

    const auto serialized=dozi::core::MasteringVersionStore{}.serialize(plan);
    const auto restored=dozi::core::MasteringVersionStore{}.deserialize(serialized);
    expect(restored.plan&&restored.plan->styleId==plan.styleId,"mastering plans round-trip through version storage");
    expect(restored.plan&&restored.plan->modules.size()==plan.modules.size()&&restored.plan->modules.front().recommendedParameters==plan.modules.front().recommendedParameters&&restored.plan->modules.front().styleParameters==plan.modules.front().styleParameters,"AUTO recommendation and RESET style snapshots round-trip through version storage");

    constexpr int rate=48000;
    dozi::core::StereoAudioBuffer audio;audio.sampleRate=rate;audio.left.resize(rate);audio.right.resize(rate);
    for(int sample=0;sample<rate;++sample){const auto value=0.4*std::sin(2.0*std::numbers::pi*80.0*sample/rate);audio.left[static_cast<std::size_t>(sample)]=value;audio.right[static_cast<std::size_t>(sample)]=value;}
    const auto before=audio.left;
    dozi::core::LinearPhaseEq{}.process(audio,{false,{{80.0,-0.5,0.7}},513});
    double beforePower=0,afterPower=0;for(std::size_t i=0;i<audio.left.size();++i){beforePower+=before[i]*before[i];afterPower+=audio.left[i]*audio.left[i];}
    expect(afterPower<beforePower,"the conversational 80 Hz adjustment reduces low-frequency energy");

    dozi::core::StereoAudioBuffer stereo;stereo.sampleRate=rate;stereo.left={1.0,0.5};stereo.right={0.0,-0.5};
    dozi::core::MasteringStereoProcessor{}.process(stereo,{false,1.0,120.0,0.0,0.0,true,true});
    expect(std::abs(stereo.left[0]-stereo.right[0])<1.0e-9,"mono check produces identical channels");

    dozi::core::StereoAudioBuffer compressorAudio;compressorAudio.sampleRate=rate;compressorAudio.left.resize(rate/10);compressorAudio.right.resize(rate/10);
    for(std::size_t i=0;i<compressorAudio.left.size();++i){const auto low=.8*std::sin(2*std::numbers::pi*40*i/rate);compressorAudio.left[i]=compressorAudio.right[i]=low;}
    auto compressorNoHpf=compressorAudio;const auto compressorOriginal=compressorAudio.left;dozi::core::BroadbandCompressor{}.process(compressorAudio,{false,-30,4,1,100,0,120,true});dozi::core::BroadbandCompressor{}.process(compressorNoHpf,{false,-30,4,1,100,0,0,true});
    double compressorDifference=0,noHpfDifference=0;for(std::size_t i=0;i<compressorAudio.left.size();++i){compressorDifference+=std::abs(compressorAudio.left[i]-compressorOriginal[i]);noHpfDifference+=std::abs(compressorNoHpf.left[i]-compressorOriginal[i]);}
    expect(compressorDifference<noHpfDifference,"sidechain HPF prevents deep bass from dominating glue compression");

    dozi::core::StereoAudioBuffer saturated; saturated.sampleRate=rate;saturated.left.resize(2048);for(std::size_t i=0;i<saturated.left.size();++i)saturated.left[i]=.65*std::sin(2*std::numbers::pi*997*i/rate);saturated.right=saturated.left;
    auto rich=saturated;dozi::core::HarmonicSaturator{}.process(saturated,{false,dozi::core::SaturationMode::clean,6,1,0,1,false,3,-.1});dozi::core::HarmonicSaturator{}.process(rich,{false,dozi::core::SaturationMode::rich,6,1,0,8,false,3,-.1});
    double characterDifference=0;for(std::size_t i=0;i<saturated.left.size();++i)characterDifference+=std::abs(saturated.left[i]-rich.left[i]);expect(characterDifference>0.01,"saturation character and HQ oversampling select distinct processing");

    dozi::core::StereoAudioBuffer limited;limited.sampleRate=rate;limited.left={1.5,0.1,0.1};limited.right={0.1,0.1,0.1};
    const auto unlinkedMetrics=dozi::core::TruePeakLimiter{}.process(limited,{false,0,-1,1,50,false,false});(void)unlinkedMetrics;
    expect(std::abs(limited.right[0]-.1)<.05,"unlinked limiting does not unnecessarily reduce the quieter channel");

    if(failures==0)std::cout<<"All Dozi Mastering tests passed\n";
    return failures==0?0:1;
}
