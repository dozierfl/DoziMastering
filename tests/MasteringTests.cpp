#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringDiagnostics.h"
#include "dozi/core/MasteringDsp.h"
#include "dozi/core/MasteringReference.h"
#include "dozi/core/MasteringProcessorChain.h"
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
    expect(plan.targetId=="streaming-normal"&&plan.targetIntegratedLufs==-14.0,"processing styles do not replace the selected delivery target");
    expect(plan.modules.size()==8,"analysis creates the complete eight-processor live mastering chain");
    const auto dynamic=std::ranges::find(plan.modules,dozi::core::MasteringModuleType::dynamicEq,&dozi::core::MasteringModulePlan::type);expect(dynamic!=plan.modules.end()&&!dynamic->bypassed&&dynamic->parameters.size()==32,"resonance analysis creates an active four-band Dynamic EQ schema");
    auto noResonanceAnalysis=analysis;noResonanceAnalysis.resonances.clear();const auto noResonancePlan=dozi::core::MasteringReferenceMatcher{}.createPhase2Plan(noResonanceAnalysis,{});const auto emptyDynamic=std::ranges::find(noResonancePlan.modules,dozi::core::MasteringModuleType::dynamicEq,&dozi::core::MasteringModulePlan::type);expect(emptyDynamic!=noResonancePlan.modules.end()&&emptyDynamic->bypassed&&emptyDynamic->parameters.size()==32,"Dynamic EQ retains four editable bands when analysis finds no resonance");
    expect(plan.truePeakCeilingDbtp<=-1.0,"AI remaster preset retains a safe true-peak ceiling");
    const auto compressor=std::ranges::find(plan.modules,dozi::core::MasteringModuleType::broadbandCompressor,&dozi::core::MasteringModulePlan::type);
    expect(compressor!=plan.modules.end()&&compressor->bypassed,"AI remaster preset avoids additional broadband compression");
    expect(std::ranges::find(plan.modules,dozi::core::MasteringModuleType::deEsser,&dozi::core::MasteringModulePlan::type)!=plan.modules.end()&&std::ranges::find(plan.modules,dozi::core::MasteringModuleType::multibandCompressor,&dozi::core::MasteringModulePlan::type)!=plan.modules.end()&&std::ranges::find(plan.modules,dozi::core::MasteringModuleType::saturation,&dozi::core::MasteringModulePlan::type)!=plan.modules.end(),"preset-addressable De-Esser, Multiband, and Saturation stages are present");

    const auto&styles=dozi::core::factoryMasteringStyles();
    expect(styles.size()==12,"all factory mastering styles are available");
    expect(dozi::core::masteringTarget("spotify-loud").has_value(),"Spotify Loud delivery target is available");
    const auto highImpact=dozi::core::masteringTarget("high-impact-8");expect(highImpact&&highImpact->integratedLufs==-8.0&&highImpact->truePeakCeilingDbtp==-1.0,"High Impact delivery preset targets -8 LUFS with a safe true-peak ceiling");

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

    dozi::core::MasteringPlan sharedPlan;sharedPlan.modules={
        {dozi::core::MasteringModuleType::linearPhaseEq,false,{{"band1.frequencyHz",1000},{"band1.gainDb",-1},{"band1.q",1}},{}},
        {dozi::core::MasteringModuleType::broadbandCompressor,false,{{"thresholdDbfs",-24},{"ratio",3},{"attackMs",10},{"releaseMs",150},{"makeupGainDb",1}},{}},
        {dozi::core::MasteringModuleType::saturation,false,{{"driveDb",2},{"mix",.5},{"character",1}},{}},
        {dozi::core::MasteringModuleType::stereoWidthMonoBass,false,{{"widthFactor",.9},{"monoBassCrossoverHz",120}},{}},
        {dozi::core::MasteringModuleType::truePeakLimiter,false,{{"inputGainDb",1},{"ceilingDbtp",-1},{"lookaheadMs",5},{"releaseMs",100}},{}},
    };dozi::core::StereoAudioBuffer liveInput;liveInput.sampleRate=rate;liveInput.left.resize(4096);liveInput.right.resize(4096);for(std::size_t i=0;i<liveInput.left.size();++i){liveInput.left[i]=.4*std::sin(2*std::numbers::pi*997*i/rate);liveInput.right[i]=.35*std::sin(2*std::numbers::pi*997*i/rate);}auto presetOutput=liveInput;dozi::core::MasteringProcessorChain presetChain;presetChain.setPlan(plan);(void)presetChain.process(presetOutput,true);double presetDifference=0;for(std::size_t i=0;i<liveInput.left.size();++i)presetDifference+=std::abs(liveInput.left[i]-presetOutput.left[i])+std::abs(liveInput.right[i]-presetOutput.right[i]);expect(presetDifference>.01,"AI WAV Remaster preset produces a measurable live Before/After difference");auto exportInput=liveInput;dozi::core::MasteringProcessorChain liveChain,exportChain;liveChain.setPlan(sharedPlan);exportChain.setPlan(sharedPlan);const auto liveTelemetry=liveChain.process(liveInput);const auto exportTelemetry=exportChain.process(exportInput);double chainDifference=0;for(std::size_t i=0;i<liveInput.left.size();++i)chainDifference+=std::abs(liveInput.left[i]-exportInput.left[i])+std::abs(liveInput.right[i]-exportInput.right[i]);expect(chainDifference<1.0e-9,"live audition and export use the identical mastering processor chain");expect(liveTelemetry.size()==sharedPlan.modules.size()&&exportTelemetry.size()==sharedPlan.modules.size(),"every processor stage emits chain telemetry in plan order");
    dozi::core::MasteringPlan statefulPlan;statefulPlan.modules={sharedPlan.modules[1],sharedPlan.modules[3]};auto continuousInput=liveInput,blockedOutput=liveInput;dozi::core::MasteringProcessorChain continuousChain,blockedChain;continuousChain.setPlan(statefulPlan);blockedChain.setPlan(statefulPlan);(void)continuousChain.process(continuousInput);for(std::size_t start=0;start<blockedOutput.left.size();start+=512){dozi::core::StereoAudioBuffer block;block.sampleRate=rate;const auto end=std::min(blockedOutput.left.size(),start+512);block.left.assign(blockedOutput.left.begin()+static_cast<std::ptrdiff_t>(start),blockedOutput.left.begin()+static_cast<std::ptrdiff_t>(end));block.right.assign(blockedOutput.right.begin()+static_cast<std::ptrdiff_t>(start),blockedOutput.right.begin()+static_cast<std::ptrdiff_t>(end));(void)blockedChain.process(block);std::copy(block.left.begin(),block.left.end(),blockedOutput.left.begin()+static_cast<std::ptrdiff_t>(start));std::copy(block.right.begin(),block.right.end(),blockedOutput.right.begin()+static_cast<std::ptrdiff_t>(start));}double blockDifference=0;for(std::size_t i=0;i<continuousInput.left.size();++i)blockDifference+=std::abs(continuousInput.left[i]-blockedOutput.left[i])+std::abs(continuousInput.right[i]-blockedOutput.right[i]);expect(blockDifference<1.0e-8,"compressor envelopes and mono-bass filters persist across playback blocks");
    auto completeContinuous=liveInput,completeBlocked=liveInput;dozi::core::MasteringProcessorChain completeContinuousChain,completeBlockedChain;completeContinuousChain.setPlan(sharedPlan);completeBlockedChain.setPlan(sharedPlan);(void)completeContinuousChain.process(completeContinuous);for(std::size_t start=0;start<completeBlocked.left.size();start+=512){dozi::core::StereoAudioBuffer block;block.sampleRate=rate;const auto end=std::min(completeBlocked.left.size(),start+512);block.left.assign(completeBlocked.left.begin()+static_cast<std::ptrdiff_t>(start),completeBlocked.left.begin()+static_cast<std::ptrdiff_t>(end));block.right.assign(completeBlocked.right.begin()+static_cast<std::ptrdiff_t>(start),completeBlocked.right.begin()+static_cast<std::ptrdiff_t>(end));(void)completeBlockedChain.process(block);std::copy(block.left.begin(),block.left.end(),completeBlocked.left.begin()+static_cast<std::ptrdiff_t>(start));std::copy(block.right.begin(),block.right.end(),completeBlocked.right.begin()+static_cast<std::ptrdiff_t>(start));}double completeBlockDifference=0;for(std::size_t i=0;i<completeContinuous.left.size();++i)completeBlockDifference+=std::abs(completeContinuous.left[i]-completeBlocked.left[i])+std::abs(completeContinuous.right[i]-completeBlocked.right[i]);expect(completeBlockDifference<1.0e-6,"complete mastering chain is invariant between full-buffer export and 512-sample live blocks");
    dozi::core::MasteringPlan dynamicsPlan;dynamicsPlan.modules={{dozi::core::MasteringModuleType::dynamicEq,false,{{"band1.frequencyHz",1000},{"band1.gainDb",-4},{"band1.q",1.2},{"band1.thresholdDbfs",-35},{"band1.attackMs",4},{"band1.releaseMs",150},{"band1.enabled",1},{"band1.listen",0}},{}},{dozi::core::MasteringModuleType::deEsser,false,{{"frequencyHz",7000},{"q",2},{"thresholdDbfs",-38},{"ratio",5},{"maximumReductionDb",5},{"attackMs",2},{"releaseMs",100},{"listen",0}},{}},{dozi::core::MasteringModuleType::multibandCompressor,false,{{"lowCrossoverHz",120},{"midCrossoverHz",700},{"highCrossoverHz",4000},{"ratio",2},{"maximumGainReductionDb",4},{"low.thresholdDbfs",-30},{"lowMid.thresholdDbfs",-30},{"highMid.thresholdDbfs",-30},{"high.thresholdDbfs",-30}},{}},{dozi::core::MasteringModuleType::truePeakLimiter,false,{{"inputGainDb",8},{"ceilingDbtp",-1},{"lookaheadMs",5},{"releaseMs",100},{"linked",1},{"truePeakEnabled",1}}, {}}};auto dynamicsContinuous=liveInput,dynamicsBlocked=liveInput;dozi::core::MasteringProcessorChain dynamicsContinuousChain,dynamicsBlockedChain;dynamicsContinuousChain.setPlan(dynamicsPlan);dynamicsBlockedChain.setPlan(dynamicsPlan);(void)dynamicsContinuousChain.process(dynamicsContinuous);for(std::size_t start=0;start<dynamicsBlocked.left.size();start+=512){dozi::core::StereoAudioBuffer block;block.sampleRate=rate;const auto end=std::min(dynamicsBlocked.left.size(),start+512);block.left.assign(dynamicsBlocked.left.begin()+static_cast<std::ptrdiff_t>(start),dynamicsBlocked.left.begin()+static_cast<std::ptrdiff_t>(end));block.right.assign(dynamicsBlocked.right.begin()+static_cast<std::ptrdiff_t>(start),dynamicsBlocked.right.begin()+static_cast<std::ptrdiff_t>(end));(void)dynamicsBlockedChain.process(block);std::copy(block.left.begin(),block.left.end(),dynamicsBlocked.left.begin()+static_cast<std::ptrdiff_t>(start));std::copy(block.right.begin(),block.right.end(),dynamicsBlocked.right.begin()+static_cast<std::ptrdiff_t>(start));}double dynamicsBlockDifference=0;for(std::size_t i=0;i<dynamicsContinuous.left.size();++i)dynamicsBlockDifference+=std::abs(dynamicsContinuous.left[i]-dynamicsBlocked.left[i])+std::abs(dynamicsContinuous.right[i]-dynamicsBlocked.right[i]);expect(dynamicsBlockDifference<1.0e-6,"dynamic, de-essing, multiband, and limiting stages are invariant across live audio blocks");

    if(failures==0)std::cout<<"All Dozi Mastering tests passed\n";
    return failures==0?0:1;
}
