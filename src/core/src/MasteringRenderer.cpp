#include "dozi/core/MasteringRenderer.h"
#include "dozi/core/MasteringDsp.h"
#include <ebur128.h>
#include <sndfile.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <system_error>
#include <stdexcept>
namespace dozi::core { namespace {
double get(const MasteringModulePlan&m,const char*k,double fallback){const auto i=m.parameters.find(k);return i==m.parameters.end()?fallback:i->second;}
double integratedLufs(const StereoAudioBuffer&audio){
 auto*state=ebur128_init(2,static_cast<unsigned long>(audio.sampleRate),EBUR128_MODE_I);if(!state)throw std::runtime_error("Loudness measurement initialization failed");
 std::vector<double>interleaved(audio.left.size()*2);for(std::size_t i=0;i<audio.left.size();++i){interleaved[2*i]=audio.left[i];interleaved[2*i+1]=audio.right[i];}
 const auto added=ebur128_add_frames_double(state,interleaved.data(),audio.left.size());double measured=-120;const auto read=added==EBUR128_SUCCESS?ebur128_loudness_global(state,&measured):EBUR128_ERROR_INVALID_MODE;ebur128_destroy(&state);return read==EBUR128_SUCCESS&&std::isfinite(measured)?measured:-120;
}
double levelDb(double value){return value<=1e-12?-120.0:20*std::log10(value);}
ProcessorTelemetry measureTelemetry(MasteringModuleType type,const StereoAudioBuffer&input,const StereoAudioBuffer&output){
 ProcessorTelemetry result;result.type=type;double inputPeak=0,outputPeak=0,inputPower=0,outputPower=0,lr=0,ll=0,rr=0;const auto frames=std::min(input.left.size(),output.left.size());
 for(std::size_t i=0;i<frames;++i){inputPeak=std::max({inputPeak,std::abs(input.left[i]),std::abs(input.right[i])});outputPeak=std::max({outputPeak,std::abs(output.left[i]),std::abs(output.right[i])});inputPower+=input.left[i]*input.left[i]+input.right[i]*input.right[i];outputPower+=output.left[i]*output.left[i]+output.right[i]*output.right[i];lr+=output.left[i]*output.right[i];ll+=output.left[i]*output.left[i];rr+=output.right[i]*output.right[i];if(std::max(std::abs(input.left[i]),std::abs(input.right[i]))>=.999)++result.peakEventCount;}
 result.inputPeakDbfs=levelDb(inputPeak);result.outputPeakDbfs=levelDb(outputPeak);result.inputRmsDbfs=levelDb(std::sqrt(inputPower/std::max<std::size_t>(1,frames*2)));result.outputRmsDbfs=levelDb(std::sqrt(outputPower/std::max<std::size_t>(1,frames*2)));result.maximumGainReductionDb=std::max(0.0,result.inputPeakDbfs-result.outputPeakDbfs);result.phaseCorrelation=ll>0&&rr>0?std::clamp(lr/std::sqrt(ll*rr),-1.0,1.0):1.0;
 const auto analysed=std::min<std::size_t>(frames,4096);for(std::size_t band=0;band<result.spectrumDbfs.size();++band){const auto frequency=20.0*std::pow(1000.0,(band+.5)/result.spectrumDbfs.size());double real=0,imaginary=0;for(std::size_t i=0;i<analysed;++i){const auto mono=(output.left[i]+output.right[i])*.5;const auto phase=2*std::numbers::pi*frequency*i/output.sampleRate;real+=mono*std::cos(phase);imaginary-=mono*std::sin(phase);}result.spectrumDbfs[band]=levelDb(analysed?2*std::hypot(real,imaginary)/analysed:0);}
 if(frames){const auto stride=std::max<std::size_t>(1,frames/128);for(std::size_t i=0;i<frames&&result.vectorscope.size()<128;i+=stride)result.vectorscope.push_back({output.left[i],output.right[i]});}
 return result;
}
}
MasteringRenderResult MasteringRenderer::render(const std::filesystem::path&source,const std::filesystem::path&output,const MasteringPlan&plan,const CancellationToken&cancel,bool auditionMode,ProcessorTelemetryCallback telemetryCallback)const{
 MasteringAnalyzer analyzer;auto before=analyzer.analyse(source,cancel);if(!before.result)throw std::runtime_error(before.error?before.error->message:"Cancelled");
 SF_INFO info{};auto*in=sf_open(source.c_str(),SFM_READ,&info);if(!in)throw std::runtime_error(sf_strerror(nullptr));StereoAudioBuffer audio{info.samplerate,{},{}};std::vector<double>block(8192);sf_count_t frames;while((frames=sf_readf_double(in,block.data(),4096))>0){if(cancel.isCancelled()){sf_close(in);throw std::runtime_error("Cancelled");}for(sf_count_t i=0;i<frames;++i){audio.left.push_back(block[2*i]);audio.right.push_back(block[2*i+1]);}}sf_close(in);
 const auto stopped=[&cancel]{return cancel.isCancelled();};
 LimiterProcessingMetrics limiterMetrics;double limiterInputGainDb=0;std::vector<ProcessorTelemetry> telemetry;
 for(const auto&m:plan.modules){
  if(stopped())throw std::runtime_error("Cancelled");
  const auto stageInput=audio;
  if(m.type==MasteringModuleType::linearPhaseEq){LinearPhaseEqSettings s;s.bypassed=m.bypassed;for(int n=1;;++n){const auto p="band"+std::to_string(n)+".";if(!m.parameters.contains(p+"frequencyHz"))break;s.bands.push_back({get(m,(p+"frequencyHz").c_str(),1000),get(m,(p+"gainDb").c_str(),0),get(m,(p+"q").c_str(),4)});}if(m.parameters.contains("assistantLowEnd.frequencyHz"))s.bands.push_back({get(m,"assistantLowEnd.frequencyHz",80),get(m,"assistantLowEnd.gainDb",0),get(m,"assistantLowEnd.q",0.7)});LinearPhaseEq{}.process(audio,s,stopped);}
  else if(m.type==MasteringModuleType::dynamicEq){DynamicEqSettings s;s.bypassed=m.bypassed;for(int n=1;;++n){const auto p="band"+std::to_string(n)+".";if(!m.parameters.contains(p+"frequencyHz"))break;const auto gain=get(m,(p+"gainDb").c_str(),0);if(gain<0)s.bands.push_back({get(m,(p+"frequencyHz").c_str(),3000),get(m,(p+"q").c_str(),1),get(m,(p+"thresholdDbfs").c_str(),-30),std::abs(gain),get(m,(p+"attackMs").c_str(),10),get(m,(p+"releaseMs").c_str(),120),get(m,(p+"enabled").c_str(),1)>.5,auditionMode&&get(m,(p+"listen").c_str(),0)>.5});}DynamicEqualizer{}.process(audio,s,stopped);}
  else if(m.type==MasteringModuleType::deEsser)DeEsser{}.process(audio,{m.bypassed,get(m,"frequencyHz",6500),get(m,"q",1),get(m,"thresholdDbfs",-30),get(m,"ratio",3),get(m,"maximumReductionDb",6),get(m,"attackMs",2),get(m,"releaseMs",80),auditionMode&&get(m,"listen",0)>.5},stopped);
  else if(m.type==MasteringModuleType::broadbandCompressor)BroadbandCompressor{}.process(audio,{m.bypassed,get(m,"thresholdDbfs",-18),get(m,"ratio",1.5),get(m,"attackMs",30),get(m,"releaseMs",150),get(m,"makeupGainDb",0),get(m,"sidechainHpfHz",0),get(m,"automaticRelease",0)>.5},stopped);
  else if(m.type==MasteringModuleType::multibandCompressor){const auto ratio=get(m,"ratio",1.5),maximum=get(m,"maximumGainReductionDb",3);const auto band=[&](const char*name,double threshold,double attack,double release){const auto p=std::string(name)+".";return MultibandCompressorBandSettings{get(m,(p+"thresholdDbfs").c_str(),threshold),get(m,(p+"ratio").c_str(),ratio),get(m,(p+"attackMs").c_str(),attack),get(m,(p+"releaseMs").c_str(),release),get(m,(p+"maximumReductionDb").c_str(),maximum),auditionMode&&get(m,(p+"solo").c_str(),0)>.5};};const auto low=band("low",-18,30,180),lowMid=band("lowMid",-18,24,160),highMid=band("highMid",-19,14,120),high=band("high",-20,8,100);MultibandCompressor{}.process(audio,{m.bypassed,get(m,"lowCrossoverHz",120),get(m,"midCrossoverHz",700),get(m,"highCrossoverHz",4000),low,lowMid,highMid,high},stopped);}
  else if(m.type==MasteringModuleType::saturation){const auto character=static_cast<int>(std::clamp(get(m,"character",get(m,"mode",1)),0.0,2.0));HarmonicSaturator{}.process(audio,{m.bypassed,static_cast<SaturationMode>(character),get(m,"driveDb",2),get(m,"mix",1),get(m,"outputTrimDb",0),static_cast<int>(get(m,"oversamplingFactor",4)),get(m,"automaticLevelCompensation",1)>0.5,get(m,"maximumCompensationDb",3),get(m,"outputCeilingDbfs",-0.1)},stopped);}
  else if(m.type==MasteringModuleType::stereoWidthMonoBass)MasteringStereoProcessor{}.process(audio,{m.bypassed,get(m,"widthFactor",1),get(m,"monoBassCrossoverHz",120),get(m,"sideTrimDb",0),get(m,"balance",0),auditionMode&&get(m,"monoCheck",0)>.5,get(m,"swapLeftRight",0)>.5});
  else if(m.type==MasteringModuleType::truePeakLimiter){
   limiterInputGainDb=get(m,"inputGainDb",0);
   if(m.bypassed)limiterMetrics=TruePeakLimiter{}.process(audio,{true,limiterInputGainDb,get(m,"ceilingDbtp",-1),get(m,"lookaheadMs",5),get(m,"releaseMs",100),get(m,"linked",1)>.5,get(m,"truePeakEnabled",1)>.5},stopped);
   else{
    const auto preLimiter=audio;auto bestAudio=audio;LimiterProcessingMetrics bestMetrics;double bestError=1.0e9,bestGain=limiterInputGainDb;
    auto candidate=std::clamp(limiterInputGainDb,-24.0,36.0);
    for(int attempt=0;attempt<10;++attempt){
     if(stopped())throw std::runtime_error("Cancelled");auto trial=preLimiter;const auto metrics=TruePeakLimiter{}.process(trial,{false,candidate,get(m,"ceilingDbtp",-1),get(m,"lookaheadMs",5),get(m,"releaseMs",100),get(m,"linked",1)>.5,get(m,"truePeakEnabled",1)>.5},stopped);const auto measured=integratedLufs(trial);const auto error=plan.targetIntegratedLufs-measured;
     if(std::abs(error)<bestError){bestError=std::abs(error);bestAudio=std::move(trial);bestMetrics=metrics;bestGain=candidate;}
     if(std::abs(error)<=0.1)break;const auto next=std::clamp(candidate+error,-24.0,36.0);if(std::abs(next-candidate)<0.01)break;candidate=next;
    }
    audio=std::move(bestAudio);limiterMetrics=bestMetrics;limiterInputGainDb=bestGain;
   }
  }
  else if(!m.bypassed)throw std::runtime_error("Mastering plan contains a Phase 2 module that is not rendered yet");
  auto measured=measureTelemetry(m.type,stageInput,audio);if(m.type==MasteringModuleType::truePeakLimiter)measured.maximumGainReductionDb=limiterMetrics.maximumGainReductionDb;if(telemetryCallback)telemetryCallback(measured);telemetry.push_back(std::move(measured));
 }
 const auto temporary=std::filesystem::path(output.string()+".partial");std::filesystem::create_directories(output.parent_path());SF_INFO outInfo{};outInfo.samplerate=audio.sampleRate;outInfo.channels=2;outInfo.format=SF_FORMAT_WAV|SF_FORMAT_FLOAT;auto*out=sf_open(temporary.c_str(),SFM_WRITE,&outInfo);if(!out)throw std::runtime_error(sf_strerror(nullptr));std::vector<double>interleaved(8192);std::size_t written=0;while(written<audio.left.size()){if(stopped()){sf_close(out);std::filesystem::remove(temporary);throw std::runtime_error("Cancelled");}const auto frames=std::min<std::size_t>(4096,audio.left.size()-written);for(std::size_t i=0;i<frames;++i){interleaved[2*i]=audio.left[written+i];interleaved[2*i+1]=audio.right[written+i];}if(sf_writef_double(out,interleaved.data(),frames)!=static_cast<sf_count_t>(frames)){sf_close(out);std::filesystem::remove(temporary);throw std::runtime_error("Incomplete mastering export");}written+=frames;}sf_close(out);std::error_code ec;std::filesystem::rename(temporary,output,ec);if(ec){std::filesystem::remove(temporary);throw std::runtime_error("Atomic mastering commit failed");}
 auto after=analyzer.analyse(output,cancel);if(!after.result){std::filesystem::remove(output);throw std::runtime_error("Rendered master failed validation");}HealthScoreConfig hc;hc.targetLufs=plan.targetIntegratedLufs;hc.truePeakCeilingDbtp=plan.truePeakCeilingDbtp;MasteringHealthScorer scorer;return{output,*before.result,*after.result,scorer.score(*before.result,hc),scorer.score(*after.result,hc),limiterMetrics.maximumGainReductionDb,limiterMetrics.finalSafetyTrimDb,limiterInputGainDb,std::move(telemetry)};}
}
