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
[[maybe_unused]] double sinc(double x){return std::abs(x)<1e-12?1.0:std::sin(std::numbers::pi*x)/(std::numbers::pi*x);}
[[maybe_unused]] std::vector<double> kernel(int rate,const LinearPhaseEqSettings&s){
 if(s.taps<3||(s.taps%2)==0)throw std::invalid_argument("EQ tap count must be odd"); std::vector<double> h(s.taps);h[s.taps/2]=1;
 for(const auto&b:s.bands){if(b.frequencyHz<=0||b.frequencyHz>=rate*.5||b.q<=0||b.gainDb<-18||b.gainDb>12)throw std::invalid_argument("Invalid corrective EQ band");
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
 double maximumCut,double attackMs,double releaseMs,DynamicBandState&state,const DspCancellation&cancel){
 if(ratio<1||maximumCut<0||attackMs<=0||releaseMs<=0)throw std::invalid_argument("Invalid dynamic processor settings");
 const auto filter=bandPass(a.sampleRate,frequency,q);
 const auto attack=std::exp(-1.0/(.001*attackMs*a.sampleRate)),release=std::exp(-1.0/(.001*releaseMs*a.sampleRate));
 for(std::size_t i=0;i<a.left.size();++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  const auto leftBand=filter.b0*a.left[i]+state.left1;state.left1=filter.b1*a.left[i]-filter.a1*leftBand+state.left2;state.left2=filter.b2*a.left[i]-filter.a2*leftBand;const auto rightBand=filter.b0*a.right[i]+state.right1;state.right1=filter.b1*a.right[i]-filter.a1*rightBand+state.right2;state.right2=filter.b2*a.right[i]-filter.a2*rightBand;
  const auto detector=std::max(std::abs(leftBand),std::abs(rightBand));
  const auto coefficient=detector>state.envelope?attack:release;state.envelope=coefficient*state.envelope+(1-coefficient)*detector;
  const auto level=dB(state.envelope),over=std::max(0.0,level-threshold);
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
 const MultibandCompressorBandSettings&s,double&gain,const DspCancellation&cancel){
 if(s.ratio<1||s.attackMs<=0||s.releaseMs<=0||s.maximumReductionDb<0)
  throw std::invalid_argument("Invalid multiband compressor settings");
 const auto attack=std::exp(-1.0/(.001*s.attackMs*rate)),release=std::exp(-1.0/(.001*s.releaseMs*rate));
 for(std::size_t i=0;i<left.size();++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  const auto level=dB(std::max(std::abs(left[i]),std::abs(right[i])));
  const auto calculated=level>s.thresholdDbfs?(s.thresholdDbfs+(level-s.thresholdDbfs)/s.ratio)-level:0;
  const auto reduction=std::max(-s.maximumReductionDb,calculated),wanted=amplitude(reduction);
  const auto coefficient=wanted<gain?attack:release;gain=coefficient*gain+(1-coefficient)*wanted;
  left[i]*=gain;right[i]*=gain;
 }
}
[[maybe_unused]] double rms(const StereoAudioBuffer&a){
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
void LinearPhaseEq::reset()noexcept{states.clear();}
void LinearPhaseEq::process(StereoAudioBuffer&a,const LinearPhaseEqSettings&s,DspCancellation cancel){valid(a);if(s.taps==0){const auto h=kernel(a.sampleRate,s);convolve(a.left,h,cancel);convolve(a.right,h,cancel);return;}if(s.bypassed||s.bands.empty()){reset();return;}states.resize(s.bands.size());for(std::size_t bandIndex=0;bandIndex<s.bands.size();++bandIndex){const auto&band=s.bands[bandIndex];if(band.frequencyHz<=0||band.frequencyHz>=a.sampleRate*.5||band.q<=0||band.gainDb<-18||band.gainDb>12)throw std::invalid_argument("Invalid corrective EQ band");const auto amplitudeFactor=std::pow(10.0,band.gainDb/40.0),omega=2*std::numbers::pi*band.frequencyHz/a.sampleRate,alpha=std::sin(omega)/(2*band.q),a0=1+alpha/amplitudeFactor;const auto b0=(1+alpha*amplitudeFactor)/a0,b1=-2*std::cos(omega)/a0,b2=(1-alpha*amplitudeFactor)/a0,a1=-2*std::cos(omega)/a0,a2=(1-alpha/amplitudeFactor)/a0;auto&state=states[bandIndex];for(std::size_t i=0;i<a.left.size();++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");const auto inputLeft=a.left[i],left=b0*inputLeft+state.left1;state.left1=b1*inputLeft-a1*left+state.left2;state.left2=b2*inputLeft-a2*left;a.left[i]=left;const auto inputRight=a.right[i],right=b0*inputRight+state.right1;state.right1=b1*inputRight-a1*right+state.right2;state.right2=b2*inputRight-a2*right;a.right[i]=right;}}}
void BroadbandCompressor::reset()noexcept{gain=1;lowL=0;lowR=0;}
void BroadbandCompressor::process(StereoAudioBuffer&a,const CompressorSettings&s,DspCancellation cancel){valid(a);if(s.bypassed){reset();return;}if(s.ratio<1||s.attackMs<=0||s.releaseMs<=0||s.sidechainHpfHz<0||s.sidechainHpfHz>=a.sampleRate*.5)throw std::invalid_argument("Invalid compressor settings");if(s.ratio<=1)gain=1;const auto attack=std::exp(-1.0/(.001*s.attackMs*a.sampleRate));const auto hpfCoefficient=s.sidechainHpfHz>0?1-std::exp(-2*std::numbers::pi*s.sidechainHpfHz/a.sampleRate):0.0;const auto automaticMakeup=s.automaticMakeup?std::clamp(std::max(0.0,-12.0-s.thresholdDbfs)*(1.0-1.0/s.ratio)*.65,0.0,12.0):0.0;for(std::size_t i=0;i<a.left.size();++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");lowL+=hpfCoefficient*(a.left[i]-lowL);lowR+=hpfCoefficient*(a.right[i]-lowR);const auto detectorL=s.sidechainHpfHz>0?a.left[i]-lowL:a.left[i],detectorR=s.sidechainHpfHz>0?a.right[i]-lowR:a.right[i];const auto level=dB(std::max(std::abs(detectorL),std::abs(detectorR)));const auto reduction=level>s.thresholdDbfs?(s.thresholdDbfs+(level-s.thresholdDbfs)/s.ratio)-level:0;const auto wanted=amplitude(reduction+s.makeupGainDb+automaticMakeup);const auto releaseMs=s.automaticRelease?s.releaseMs*(1.0+3.0*std::clamp(1.0-gain,0.0,1.0)):s.releaseMs;const auto release=std::exp(-1.0/(.001*releaseMs*a.sampleRate));const auto c=wanted<gain?attack:release;gain=c*gain+(1-c)*wanted;a.left[i]*=gain;a.right[i]*=gain;}}
void DynamicEqualizer::reset()noexcept{states.clear();}
void DynamicEqualizer::process(StereoAudioBuffer&a,const DynamicEqSettings&s,DspCancellation cancel){valid(a);if(s.bypassed){reset();return;}states.resize(s.bands.size());const auto listening=std::ranges::any_of(s.bands,[](const auto&b){return b.enabled&&b.listen;});const auto original=listening?std::optional<StereoAudioBuffer>(a):std::nullopt;for(std::size_t index=0;index<s.bands.size();++index){const auto&b=s.bands[index];if(b.enabled&&(!listening||b.listen))dynamicBand(a,b.frequencyHz,b.q,b.thresholdDbfs,1000,b.maximumCutDb,b.attackMs,b.releaseMs,states[index],cancel);}if(original)for(std::size_t i=0;i<a.left.size();++i){a.left[i]=original->left[i]-a.left[i];a.right[i]=original->right[i]-a.right[i];}}
void DeEsser::reset()noexcept{state={};}
void DeEsser::process(StereoAudioBuffer&a,const DeEsserSettings&s,DspCancellation cancel){valid(a);if(s.bypassed){reset();return;}const auto original=s.listen?std::optional<StereoAudioBuffer>(a):std::nullopt;dynamicBand(a,s.frequencyHz,s.q,s.thresholdDbfs,s.ratio,s.maximumReductionDb,s.attackMs,s.releaseMs,state,cancel);if(original)for(std::size_t i=0;i<a.left.size();++i){a.left[i]=original->left[i]-a.left[i];a.right[i]=original->right[i]-a.right[i];}}
void MultibandCompressor::reset()noexcept{crossoverStates.fill(0);gains.fill(1);}
void MultibandCompressor::process(StereoAudioBuffer&a,const MultibandCompressorSettings&s,DspCancellation cancel){
 valid(a);if(s.bypassed){reset();return;}if(s.lowCrossoverHz>=s.midCrossoverHz||s.midCrossoverHz>=s.highCrossoverHz)throw std::invalid_argument("Multiband crossovers must be ordered");
 const auto frames=a.left.size();for(auto&buffer:scratch)buffer.resize(frames);auto&lowL=scratch[0];auto&lowR=scratch[1];auto&lowMidL=scratch[2];auto&lowMidR=scratch[3];auto&highMidL=scratch[4];auto&highMidR=scratch[5];auto&highL=scratch[6];auto&highR=scratch[7];
 const auto lowCoefficient=lowPass(a.sampleRate,s.lowCrossoverHz).coefficient,midCoefficient=lowPass(a.sampleRate,s.midCrossoverHz).coefficient,highCoefficient=lowPass(a.sampleRate,s.highCrossoverHz).coefficient;
 for(std::size_t i=0;i<frames;++i){
  if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");
  crossoverStates[0]+=lowCoefficient*(a.left[i]-crossoverStates[0]);crossoverStates[1]+=lowCoefficient*(a.right[i]-crossoverStates[1]);lowL[i]=crossoverStates[0];lowR[i]=crossoverStates[1];
  crossoverStates[2]+=midCoefficient*(a.left[i]-crossoverStates[2]);crossoverStates[3]+=midCoefficient*(a.right[i]-crossoverStates[3]);const auto belowMidL=crossoverStates[2],belowMidR=crossoverStates[3];
  crossoverStates[4]+=highCoefficient*(a.left[i]-crossoverStates[4]);crossoverStates[5]+=highCoefficient*(a.right[i]-crossoverStates[5]);const auto belowHighL=crossoverStates[4],belowHighR=crossoverStates[5];
  lowMidL[i]=belowMidL-lowL[i];lowMidR[i]=belowMidR-lowR[i];highMidL[i]=belowHighL-belowMidL;highMidR[i]=belowHighR-belowMidR;highL[i]=a.left[i]-belowHighL;highR[i]=a.right[i]-belowHighR;
 }
 compressBands(lowL,lowR,a.sampleRate,s.low,gains[0],cancel);compressBands(lowMidL,lowMidR,a.sampleRate,s.lowMid,gains[1],cancel);compressBands(highMidL,highMidR,a.sampleRate,s.highMid,gains[2],cancel);compressBands(highL,highR,a.sampleRate,s.high,gains[3],cancel);
 const auto anySolo=s.low.solo||s.lowMid.solo||s.highMid.solo||s.high.solo;
 for(std::size_t i=0;i<frames;++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");a.left[i]=(anySolo&&!s.low.solo?0:lowL[i])+(anySolo&&!s.lowMid.solo?0:lowMidL[i])+(anySolo&&!s.highMid.solo?0:highMidL[i])+(anySolo&&!s.high.solo?0:highL[i]);a.right[i]=(anySolo&&!s.low.solo?0:lowR[i])+(anySolo&&!s.lowMid.solo?0:lowMidR[i])+(anySolo&&!s.highMid.solo?0:highMidR[i])+(anySolo&&!s.high.solo?0:highR[i]);}
}
void HarmonicSaturator::reset()noexcept{previousLeft=previousRight=inputPower=outputPower=dcLeft=dcRight=0;}
void HarmonicSaturator::process(StereoAudioBuffer&a,const SaturationSettings&s,DspCancellation cancel){valid(a);if(s.bypassed){reset();return;}if(s.mix<0||s.mix>1||s.maximumCompensationDb<0||s.outputCeilingDbfs>0||(s.oversamplingFactor!=1&&s.oversamplingFactor!=2&&s.oversamplingFactor!=4&&s.oversamplingFactor!=8))throw std::invalid_argument("Invalid saturation settings");const auto drive=amplitude(s.driveDb),ceiling=amplitude(s.outputCeilingDbfs),meterCoefficient=1-std::exp(-1.0/(.05*a.sampleRate)),dcCoefficient=1-std::exp(-2*std::numbers::pi*5.0/a.sampleRate);for(std::size_t i=0;i<a.left.size();++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");const auto dryLeft=a.left[i],dryRight=a.right[i];double wetLeft=0,wetRight=0;for(int phase=1;phase<=s.oversamplingFactor;++phase){const auto fraction=static_cast<double>(phase)/s.oversamplingFactor;wetLeft+=shape(previousLeft+(dryLeft-previousLeft)*fraction,s.mode,drive);wetRight+=shape(previousRight+(dryRight-previousRight)*fraction,s.mode,drive);}wetLeft/=s.oversamplingFactor;wetRight/=s.oversamplingFactor;previousLeft=dryLeft;previousRight=dryRight;dcLeft+=dcCoefficient*(wetLeft-dcLeft);dcRight+=dcCoefficient*(wetRight-dcRight);wetLeft-=dcLeft;wetRight-=dcRight;auto outputLeft=dryLeft*(1-s.mix)+wetLeft*s.mix,outputRight=dryRight*(1-s.mix)+wetRight*s.mix;inputPower+=meterCoefficient*((dryLeft*dryLeft+dryRight*dryRight)*.5-inputPower);outputPower+=meterCoefficient*((outputLeft*outputLeft+outputRight*outputRight)*.5-outputPower);auto compensation=s.outputTrimDb;if(s.automaticLevelCompensation&&inputPower>1e-12&&outputPower>1e-12)compensation+=std::clamp(10*std::log10(inputPower/outputPower),-s.maximumCompensationDb,s.maximumCompensationDb);const auto gain=amplitude(compensation);a.left[i]=std::clamp(outputLeft*gain,-ceiling,ceiling);a.right[i]=std::clamp(outputRight*gain,-ceiling,ceiling);}}
double TruePeakLimiter::truePeakDbtp(const StereoAudioBuffer&a)const{valid(a);auto*state=ebur128_init(2,a.sampleRate,EBUR128_MODE_TRUE_PEAK);if(!state)throw std::runtime_error("libebur128 init failed");std::vector<double>v(a.left.size()*2);for(std::size_t i=0;i<a.left.size();++i){v[2*i]=a.left[i];v[2*i+1]=a.right[i];}ebur128_add_frames_double(state,v.data(),a.left.size());double peak=0,p=0;for(unsigned c=0;c<2;++c)if(ebur128_true_peak(state,c,&p)==EBUR128_SUCCESS)peak=std::max(peak,p);ebur128_destroy(&state);return dB(peak);}
void TruePeakLimiter::reset()noexcept{linkedGain=leftGain=rightGain=1;}
LimiterProcessingMetrics TruePeakLimiter::process(StereoAudioBuffer&a,const LimiterSettings&s,DspCancellation cancel){valid(a);LimiterProcessingMetrics metrics;if(s.bypassed){reset();return metrics;}if(s.ceilingDbtp>0||s.lookaheadMs<=0||s.releaseMs<=0)throw std::invalid_argument("Invalid limiter settings");const auto input=amplitude(s.inputGainDb),nominalCeiling=amplitude(s.ceilingDbtp),safetyMarginDb=(s.truePeakEnabled?.15:0.0)+std::min(.2,s.lookaheadMs*.02),detectorCeiling=amplitude(s.ceilingDbtp-safetyMarginDb),release=std::exp(-1.0/(s.releaseMs*.001*a.sampleRate));double minimumGain=1;for(std::size_t i=0;i<a.left.size();++i){if((i%4096)==0&&cancel&&cancel())throw std::runtime_error("Cancelled");const auto left=a.left[i]*input,right=a.right[i]*input;if(s.linked){const auto peak=std::max(std::abs(left),std::abs(right)),wanted=peak>detectorCeiling?detectorCeiling/peak:1.0;linkedGain=wanted<linkedGain?wanted:release*linkedGain+(1-release)*wanted;minimumGain=std::min(minimumGain,linkedGain);a.left[i]=std::clamp(left*linkedGain,-nominalCeiling,nominalCeiling);a.right[i]=std::clamp(right*linkedGain,-nominalCeiling,nominalCeiling);}else{const auto wantedLeft=std::abs(left)>detectorCeiling?detectorCeiling/std::abs(left):1.0,wantedRight=std::abs(right)>detectorCeiling?detectorCeiling/std::abs(right):1.0;leftGain=wantedLeft<leftGain?wantedLeft:release*leftGain+(1-release)*wantedLeft;rightGain=wantedRight<rightGain?wantedRight:release*rightGain+(1-release)*wantedRight;minimumGain=std::min({minimumGain,leftGain,rightGain});a.left[i]=std::clamp(left*leftGain,-nominalCeiling,nominalCeiling);a.right[i]=std::clamp(right*rightGain,-nominalCeiling,nominalCeiling);}}metrics.maximumGainReductionDb=-dB(minimumGain);return metrics;}
}
