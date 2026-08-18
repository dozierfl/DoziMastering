#include "dozi/core/MasteringDsp.h"
#include <ebur128.h>
#include <deque>
#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <optional>
#include <stdexcept>
namespace dozi::core { namespace {
double amplitude(double dB){return std::pow(10.0,dB/20.0);} double dB(double v){return v<=1e-12?-120:20*std::log10(v);}
void valid(const StereoAudioBuffer&a){if(a.sampleRate<=0||a.left.size()!=a.right.size())throw std::invalid_argument("Invalid stereo buffer");}
double sinc(double x){return std::abs(x)<1e-12?1.0:std::sin(std::numbers::pi*x)/(std::numbers::pi*x);}
std::vector<double> kernel(int rate,const LinearPhaseEqSettings&s){
 if(s.taps<3||(s.taps%2)==0)throw std::invalid_argument("EQ tap count must be odd"); std::vector<double> h(s.taps);h[s.taps/2]=1;
 for(const auto&b:s.bands){if(b.frequencyHz<=0||b.frequencyHz>=rate*.5||b.q<=0||b.gainDb>0)throw std::invalid_argument("Invalid corrective EQ band");
  const double width=b.frequencyHz/b.q,lo=std::max(5.0,b.frequencyHz-width*.5)/rate,hi=std::min(rate*.499,b.frequencyHz+width*.5)/rate;
  for(int n=0;n<s.taps;++n){const double x=n-s.taps/2.0;const double bp=2*hi*sinc(2*hi*x)-2*lo*sinc(2*lo*x);const double w=.5-.5*std::cos(2*std::numbers::pi*n/(s.taps-1));h[n]+=(amplitude(b.gainDb)-1)*bp*w;}}
 return h;}
void fft(std::vector<std::complex<double>>&v,bool inverse){for(std::size_t i=1,j=0;i<v.size();++i){auto bit=v.size()>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(v[i],v[j]);}for(std::size_t len=2;len<=v.size();len<<=1){const auto angle=(inverse?2:-2)*std::numbers::pi/len;const std::complex<double>wlen(std::cos(angle),std::sin(angle));for(std::size_t i=0;i<v.size();i+=len){std::complex<double>w(1);for(std::size_t j=0;j<len/2;++j){const auto u=v[i+j],z=v[i+j+len/2]*w;v[i+j]=u+z;v[i+j+len/2]=u-z;w*=wlen;}}}if(inverse)for(auto&z:v)z/=v.size();}
void convolve(std::vector<double>&x,const std::vector<double>&h,const DspCancellation&cancel){const auto originalSize=x.size();constexpr std::size_t block=16384;std::size_t fftSize=1;while(fftSize<block+h.size()-1)fftSize<<=1;std::vector<std::complex<double>>filter(fftSize);for(std::size_t i=0;i<h.size();++i)filter[i]=h[i];fft(filter,false);std::vector<double>output(originalSize+h.size()-1);for(std::size_t start=0;start<originalSize;start+=block){if(cancel&&cancel())throw std::runtime_error("Cancelled");const auto count=std::min(block,originalSize-start);std::vector<std::complex<double>>values(fftSize);for(std::size_t i=0;i<count;++i)values[i]=x[start+i];fft(values,false);for(std::size_t i=0;i<fftSize;++i)values[i]*=filter[i];fft(values,true);const auto available=std::min(fftSize,output.size()-start);for(std::size_t i=0;i<available;++i)output[start+i]+=values[i].real();}const auto delay=h.size()/2;for(std::size_t i=0;i<originalSize;++i)x[i]=output[i+delay];}
struct Biquad {
 double b0=0,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 double process(double x){const auto y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
};
Biquad bandPass(int rate,double frequency,double q){
 if(frequency<=0||frequency>=rate*.5||q<=0)throw std::invalid_argument("Invalid dynamic filter settings");
 const auto w=2*std::numbers::pi*frequency/rate,alpha=std::sin(w)/(2*q),a0=1+alpha;
 return {alpha/a0,0,-alpha/a0,-2*std::cos(w)/a0,(1-alpha)/a0};
}
void dynamicBand(StereoAudioBuffer&a,double frequency,double q,double threshold,double ratio,
 double maximumCut,double attackMs,double releaseMs,const DspCancellation&cancel){
 if(ratio<1||maximumCut<0||attackMs<=0||releaseMs<=0)throw std::invalid_argument("Invalid dynamic processor settings");
 auto leftFilter=bandPass(a.sampleRate,frequency,q),rightFilter=bandPass(a.sampleRate,frequency,q);
 const auto attack=std::exp(-1.0/(.001*attackMs*a.sampleRate)),release=std::exp(-1.0/(.001*releaseMs*a.sampleRate));
 double envelope=0;
 for(std::size_t i=0;i<a.left.size();++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  const auto leftBand=leftFilter.process(a.left[i]),rightBand=rightFilter.process(a.right[i]);
  const auto detector=std::max(std::abs(leftBand),std::abs(rightBand));
  const auto coefficient=detector>envelope?attack:release;envelope=coefficient*envelope+(1-coefficient)*detector;
  const auto level=dB(envelope),over=std::max(0.0,level-threshold);
  const auto reduction=std::min(maximumCut,over*(1-1/ratio));
  const auto removed=1-amplitude(-reduction);
  a.left[i]-=leftBand*removed;a.right[i]-=rightBand*removed;
 }
}
struct OnePoleLowPass {
 double coefficient=0,state=0;
 double process(double input){state+=coefficient*(input-state);return state;}
};
OnePoleLowPass lowPass(int rate,double frequency){
 if(frequency<=0||frequency>=rate*.5)throw std::invalid_argument("Invalid crossover frequency");
 return {1-std::exp(-2*std::numbers::pi*frequency/rate),0};
}
void compressBands(std::vector<double>&left,std::vector<double>&right,int rate,
 const MultibandCompressorBandSettings&s,const DspCancellation&cancel){
 if(s.ratio<1||s.attackMs<=0||s.releaseMs<=0||s.maximumReductionDb<0)
  throw std::invalid_argument("Invalid multiband compressor settings");
 const auto attack=std::exp(-1.0/(.001*s.attackMs*rate)),release=std::exp(-1.0/(.001*s.releaseMs*rate));
 double gain=1;
 for(std::size_t i=0;i<left.size();++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  const auto level=dB(std::max(std::abs(left[i]),std::abs(right[i])));
  const auto calculated=level>s.thresholdDbfs?(s.thresholdDbfs+(level-s.thresholdDbfs)/s.ratio)-level:0;
  const auto reduction=std::max(-s.maximumReductionDb,calculated),wanted=amplitude(reduction);
  const auto coefficient=wanted<gain?attack:release;gain=coefficient*gain+(1-coefficient)*wanted;
  left[i]*=gain;right[i]*=gain;
 }
}
double rms(const StereoAudioBuffer&a){
 if(a.left.empty())return 0;double sum=0;for(std::size_t i=0;i<a.left.size();++i)sum+=a.left[i]*a.left[i]+a.right[i]*a.right[i];
 return std::sqrt(sum/(2*a.left.size()));
}
double shape(double input,SaturationMode mode,double drive){
 if(mode==SaturationMode::clean)return std::atan(input*drive)/std::atan(drive);
 if(mode==SaturationMode::warm)return std::tanh(input*drive)/std::tanh(drive);
 const auto biased=input*drive+0.22*input*input*drive+0.04*input*input*input*drive;
 return std::tanh(biased)/std::tanh(drive);
}
}
void LinearPhaseEq::process(StereoAudioBuffer&a,const LinearPhaseEqSettings&s,DspCancellation cancel)const{valid(a);if(s.bypassed||s.bands.empty())return;const auto h=kernel(a.sampleRate,s);convolve(a.left,h,cancel);convolve(a.right,h,cancel);}
void BroadbandCompressor::process(StereoAudioBuffer&a,const CompressorSettings&s,DspCancellation cancel)const{valid(a);if(s.bypassed)return;if(s.ratio<1||s.attackMs<=0||s.releaseMs<=0||s.sidechainHpfHz<0||s.sidechainHpfHz>=a.sampleRate*.5)throw std::invalid_argument("Invalid compressor settings");double gain=1,lowL=0,lowR=0;const auto attack=std::exp(-1.0/(.001*s.attackMs*a.sampleRate));const auto hpfCoefficient=s.sidechainHpfHz>0?1-std::exp(-2*std::numbers::pi*s.sidechainHpfHz/a.sampleRate):0.0;for(std::size_t i=0;i<a.left.size();++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");lowL+=hpfCoefficient*(a.left[i]-lowL);lowR+=hpfCoefficient*(a.right[i]-lowR);const auto detectorL=s.sidechainHpfHz>0?a.left[i]-lowL:a.left[i],detectorR=s.sidechainHpfHz>0?a.right[i]-lowR:a.right[i];const auto level=dB(std::max(std::abs(detectorL),std::abs(detectorR)));const auto reduction=level>s.thresholdDbfs?(s.thresholdDbfs+(level-s.thresholdDbfs)/s.ratio)-level:0;const auto wanted=amplitude(reduction+s.makeupGainDb);const auto releaseMs=s.automaticRelease?s.releaseMs*(1.0+3.0*std::clamp(1.0-gain,0.0,1.0)):s.releaseMs;const auto release=std::exp(-1.0/(.001*releaseMs*a.sampleRate));const auto c=wanted<gain?attack:release;gain=c*gain+(1-c)*wanted;a.left[i]*=gain;a.right[i]*=gain;}}
void DynamicEqualizer::process(StereoAudioBuffer&a,const DynamicEqSettings&s,DspCancellation cancel)const{valid(a);if(s.bypassed)return;const auto original=a;const auto listening=std::ranges::any_of(s.bands,[](const auto&b){return b.enabled&&b.listen;});for(const auto&b:s.bands)if(b.enabled&&(!listening||b.listen))dynamicBand(a,b.frequencyHz,b.q,b.thresholdDbfs,1000,b.maximumCutDb,b.attackMs,b.releaseMs,cancel);if(listening)for(std::size_t i=0;i<a.left.size();++i){a.left[i]=original.left[i]-a.left[i];a.right[i]=original.right[i]-a.right[i];}}
void DeEsser::process(StereoAudioBuffer&a,const DeEsserSettings&s,DspCancellation cancel)const{valid(a);if(s.bypassed)return;const auto original=s.listen?std::optional<StereoAudioBuffer>(a):std::nullopt;dynamicBand(a,s.frequencyHz,s.q,s.thresholdDbfs,s.ratio,s.maximumReductionDb,s.attackMs,s.releaseMs,cancel);if(original)for(std::size_t i=0;i<a.left.size();++i){a.left[i]=original->left[i]-a.left[i];a.right[i]=original->right[i]-a.right[i];}}
void MultibandCompressor::process(StereoAudioBuffer&a,const MultibandCompressorSettings&s,DspCancellation cancel)const{
 valid(a);if(s.bypassed)return;if(s.lowCrossoverHz>=s.midCrossoverHz||s.midCrossoverHz>=s.highCrossoverHz)throw std::invalid_argument("Multiband crossovers must be ordered");
 const auto frames=a.left.size();std::vector<double>lowL(frames),lowR(frames),lowMidL(frames),lowMidR(frames),highMidL(frames),highMidR(frames),highL(frames),highR(frames);
 auto lowSplitL=lowPass(a.sampleRate,s.lowCrossoverHz),lowSplitR=lowPass(a.sampleRate,s.lowCrossoverHz);
 auto midSplitL=lowPass(a.sampleRate,s.midCrossoverHz),midSplitR=lowPass(a.sampleRate,s.midCrossoverHz);
 auto highSplitL=lowPass(a.sampleRate,s.highCrossoverHz),highSplitR=lowPass(a.sampleRate,s.highCrossoverHz);
 for(std::size_t i=0;i<frames;++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  lowL[i]=lowSplitL.process(a.left[i]);lowR[i]=lowSplitR.process(a.right[i]);
  const auto belowMidL=midSplitL.process(a.left[i]),belowMidR=midSplitR.process(a.right[i]);
  const auto belowHighL=highSplitL.process(a.left[i]),belowHighR=highSplitR.process(a.right[i]);
  lowMidL[i]=belowMidL-lowL[i];lowMidR[i]=belowMidR-lowR[i];highMidL[i]=belowHighL-belowMidL;highMidR[i]=belowHighR-belowMidR;highL[i]=a.left[i]-belowHighL;highR[i]=a.right[i]-belowHighR;
 }
 compressBands(lowL,lowR,a.sampleRate,s.low,cancel);compressBands(lowMidL,lowMidR,a.sampleRate,s.lowMid,cancel);compressBands(highMidL,highMidR,a.sampleRate,s.highMid,cancel);compressBands(highL,highR,a.sampleRate,s.high,cancel);
 const auto anySolo=s.low.solo||s.lowMid.solo||s.highMid.solo||s.high.solo;
 for(std::size_t i=0;i<frames;++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");a.left[i]=(anySolo&&!s.low.solo?0:lowL[i])+(anySolo&&!s.lowMid.solo?0:lowMidL[i])+(anySolo&&!s.highMid.solo?0:highMidL[i])+(anySolo&&!s.high.solo?0:highL[i]);a.right[i]=(anySolo&&!s.low.solo?0:lowR[i])+(anySolo&&!s.lowMid.solo?0:lowMidR[i])+(anySolo&&!s.highMid.solo?0:highMidR[i])+(anySolo&&!s.high.solo?0:highR[i]);}
}
void HarmonicSaturator::process(StereoAudioBuffer&a,const SaturationSettings&s,DspCancellation cancel)const{
 valid(a);if(s.bypassed)return;if(s.mix<0||s.mix>1||s.maximumCompensationDb<0||s.outputCeilingDbfs>0||(s.oversamplingFactor!=1&&s.oversamplingFactor!=2&&s.oversamplingFactor!=4&&s.oversamplingFactor!=8))
  throw std::invalid_argument("Invalid saturation settings");
 const auto inputRms=rms(a),drive=amplitude(s.driveDb),ceiling=amplitude(s.outputCeilingDbfs);
 double previousLeft=a.left.empty()?0:a.left.front(),previousRight=a.right.empty()?0:a.right.front();
 double dcLeft=0,dcRight=0;
 for(std::size_t i=0;i<a.left.size();++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  double wetLeft=0,wetRight=0;
  for(int phase=1;phase<=s.oversamplingFactor;++phase){const auto fraction=static_cast<double>(phase)/s.oversamplingFactor;wetLeft+=shape(previousLeft+(a.left[i]-previousLeft)*fraction,s.mode,drive);wetRight+=shape(previousRight+(a.right[i]-previousRight)*fraction,s.mode,drive);}
  wetLeft/=s.oversamplingFactor;wetRight/=s.oversamplingFactor;previousLeft=a.left[i];previousRight=a.right[i];dcLeft+=wetLeft;dcRight+=wetRight;
  a.left[i]=a.left[i]*(1-s.mix)+wetLeft*s.mix;a.right[i]=a.right[i]*(1-s.mix)+wetRight*s.mix;
 }
 if(!a.left.empty()){dcLeft/=a.left.size();dcRight/=a.right.size();for(std::size_t i=0;i<a.left.size();++i){a.left[i]-=dcLeft*s.mix;a.right[i]-=dcRight*s.mix;}}
 auto compensation=s.outputTrimDb;
 const auto outputRms=rms(a);if(s.automaticLevelCompensation&&inputRms>1e-12&&outputRms>1e-12)
  compensation+=std::clamp(dB(inputRms/outputRms),-s.maximumCompensationDb,s.maximumCompensationDb);
 const auto gain=amplitude(compensation);
 for(std::size_t i=0;i<a.left.size();++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");a.left[i]=std::clamp(a.left[i]*gain,-ceiling,ceiling);a.right[i]=std::clamp(a.right[i]*gain,-ceiling,ceiling);}
}
double TruePeakLimiter::truePeakDbtp(const StereoAudioBuffer&a)const{valid(a);auto*state=ebur128_init(2,a.sampleRate,EBUR128_MODE_TRUE_PEAK);if(!state)throw std::runtime_error("libebur128 init failed");std::vector<double>v(a.left.size()*2);for(std::size_t i=0;i<a.left.size();++i){v[2*i]=a.left[i];v[2*i+1]=a.right[i];}ebur128_add_frames_double(state,v.data(),a.left.size());double peak=0,p=0;for(unsigned c=0;c<2;++c)if(ebur128_true_peak(state,c,&p)==EBUR128_SUCCESS)peak=std::max(peak,p);ebur128_destroy(&state);return dB(peak);}
LimiterProcessingMetrics TruePeakLimiter::process(StereoAudioBuffer&a,const LimiterSettings&s,DspCancellation cancel)const{
 valid(a);LimiterProcessingMetrics metrics;if(s.bypassed)return metrics;if(s.ceilingDbtp>0||s.lookaheadMs<=0||s.releaseMs<=0)throw std::invalid_argument("Invalid limiter settings");
 const auto input=amplitude(s.inputGainDb),ceiling=amplitude(s.ceilingDbtp);const auto frames=a.left.size();std::vector<double>peaks(frames);
 for(std::size_t i=0;i<frames;++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");a.left[i]*=input;a.right[i]*=input;peaks[i]=std::max(std::abs(a.left[i]),std::abs(a.right[i]));}
 const auto lookahead=std::max<std::size_t>(1,static_cast<std::size_t>(std::llround(s.lookaheadMs*.001*a.sampleRate)));const auto release=std::exp(-1.0/(s.releaseMs*.001*a.sampleRate));std::deque<std::size_t> maximum;std::size_t added=0;double gain=1,minGain=1;
 if(s.linked){for(std::size_t i=0;i<frames;++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");const auto end=std::min(frames,i+lookahead+1);while(added<end){while(!maximum.empty()&&peaks[maximum.back()]<=peaks[added])maximum.pop_back();maximum.push_back(added++);}while(!maximum.empty()&&maximum.front()<i)maximum.pop_front();const auto futurePeak=maximum.empty()?0:peaks[maximum.front()];const auto wanted=futurePeak>ceiling?ceiling/futurePeak:1.0;gain=wanted<gain?wanted:release*gain+(1-release)*wanted;minGain=std::min(minGain,gain);a.left[i]*=gain;a.right[i]*=gain;}}
 else{const auto limitChannel=[&](std::vector<double>&channel){std::vector<double>channelPeaks(frames);for(std::size_t i=0;i<frames;++i)channelPeaks[i]=std::abs(channel[i]);std::deque<std::size_t>queue;std::size_t next=0;double channelGain=1.0,minimum=1.0;for(std::size_t i=0;i<frames;++i){const auto end=std::min(frames,i+lookahead+1);while(next<end){while(!queue.empty()&&channelPeaks[queue.back()]<=channelPeaks[next])queue.pop_back();queue.push_back(next++);}while(!queue.empty()&&queue.front()<i)queue.pop_front();const auto future=queue.empty()?0:channelPeaks[queue.front()];const auto wanted=future>ceiling?ceiling/future:1.0;channelGain=wanted<channelGain?wanted:release*channelGain+(1-release)*wanted;minimum=std::min(minimum,channelGain);channel[i]*=channelGain;}return minimum;};minGain=std::min(limitChannel(a.left),limitChannel(a.right));}
 metrics.maximumGainReductionDb=-dB(minGain);double measured;if(s.truePeakEnabled)measured=truePeakDbtp(a);else{double samplePeak=0;for(std::size_t i=0;i<frames;++i)samplePeak=std::max({samplePeak,std::abs(a.left[i]),std::abs(a.right[i])});measured=dB(samplePeak);}if(measured>s.ceilingDbtp){metrics.finalSafetyTrimDb=s.ceilingDbtp-measured;const auto trim=amplitude(metrics.finalSafetyTrimDb);for(std::size_t i=0;i<frames;++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");a.left[i]*=trim;a.right[i]*=trim;}metrics.maximumGainReductionDb-=metrics.finalSafetyTrimDb;}return metrics;
}
}
