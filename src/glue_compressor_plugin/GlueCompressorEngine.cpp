#include "GlueCompressorEngine.h"

#include <algorithm>
#include <cmath>

namespace {
float coefficient(float milliseconds, double rate) {
    return std::exp(-1.0f / (0.001f * milliseconds * static_cast<float>(rate)));
}
float db(float amplitude) { return juce::Decibels::gainToDecibels(amplitude, -100.0f); }
}
void GlueCompressorEngine::prepare(double sampleRate, int, int) { rate=sampleRate; reset(); }
void GlueCompressorEngine::reset(){detectorEnvelopeSquared=0;reductionEnvelopeDb=0;sustainedReductionDb=0;hpf={};}

float GlueCompressorEngine::softKneeGainReduction(float input, float threshold, float ratio) {
    ratio=std::max(1.0f,ratio);constexpr float knee=3.0f;const auto over=input-threshold;
    if(over<=-knee*.5f)return 0.0f;
    const auto slope=1.0f-1.0f/ratio;
    if(over>=knee*.5f)return over*slope;
    const auto kneeInput=over+knee*.5f;
    return slope*kneeInput*kneeInput/(2.0f*knee);
}

float GlueCompressorEngine::automaticMakeupDb(float threshold, float ratio) {
    const auto nominalOver=std::max(0.0f,-12.0f-threshold);
    return std::clamp(nominalOver*(1.0f-1.0f/std::max(1.0f,ratio))*.65f,0.0f,12.0f);
}

float GlueCompressorEngine::detectorHpf(float x,int channel){
    // 2nd-order Butterworth high-pass at 80 Hz; detector path only.
    const auto w=std::tan(juce::MathConstants<double>::pi*80.0/rate);const auto norm=1.0/(1.0+std::sqrt(2.0)*w+w*w);
    const auto b0=norm,b1=-2.0*norm,b2=norm,a1=2.0*(w*w-1.0)*norm,a2=(1.0-std::sqrt(2.0)*w+w*w)*norm;
    auto&s=hpf[static_cast<std::size_t>(std::clamp(channel,0,1))];const auto y=static_cast<float>(b0*x+b1*s.x1+b2*s.x2-a1*s.y1-a2*s.y2);s.x2=s.x1;s.x1=x;s.y2=s.y1;s.y1=y;return y;
}

GlueCompressorLevels GlueCompressorEngine::process(juce::AudioBuffer<float>&buffer,const GlueCompressorSettings&s){
    GlueCompressorLevels levels;const auto channels=std::min(2,buffer.getNumChannels());if(channels==0)return levels;
    const auto rmsCoefficient=coefficient(10.0f,rate);const auto attackCoefficient=coefficient(std::clamp(s.attackMs,.1f,100.0f),rate);
    const auto manualRelease=std::clamp(s.releaseMs,50.0f,1000.0f);const auto makeup=s.makeupGainDb+(s.autoMakeup?automaticMakeupDb(s.thresholdDbfs,s.ratio):0.0f);
    for(int sample=0;sample<buffer.getNumSamples();++sample){
        float detector=0.0f;for(int channel=0;channel<channels;++channel){const auto x=buffer.getSample(channel,sample);levels.inputDbfs[static_cast<std::size_t>(channel)]=std::max(levels.inputDbfs[static_cast<std::size_t>(channel)],db(std::abs(x)));const auto side=s.sidechainHpf?detectorHpf(x,channel):x;detector=std::max(detector,std::abs(side));}
        detectorEnvelopeSquared=rmsCoefficient*detectorEnvelopeSquared+(1.0f-rmsCoefficient)*detector*detector;const auto detectorDb=db(std::sqrt(std::max(0.0f,detectorEnvelopeSquared)));const auto wanted=softKneeGainReduction(detectorDb,std::clamp(s.thresholdDbfs,-40.0f,10.0f),std::clamp(s.ratio,1.0f,20.0f));
        sustainedReductionDb=coefficient(600.0f,rate)*sustainedReductionDb+(1.0f-coefficient(600.0f,rate))*wanted;float release=manualRelease;if(s.autoRelease){const auto blend=std::clamp(sustainedReductionDb/12.0f,0.0f,1.0f);release=50.0f+blend*1450.0f;}const auto releaseCoefficient=coefficient(release,rate);reductionEnvelopeDb=(wanted>reductionEnvelopeDb?attackCoefficient:releaseCoefficient)*reductionEnvelopeDb+(1.0f-(wanted>reductionEnvelopeDb?attackCoefficient:releaseCoefficient))*wanted;
        const auto gain=s.moduleEnabled?juce::Decibels::decibelsToGain(-reductionEnvelopeDb+makeup):1.0f;for(int channel=0;channel<channels;++channel){auto y=buffer.getSample(channel,sample)*gain;buffer.setSample(channel,sample,y);levels.outputDbfs[static_cast<std::size_t>(channel)]=std::max(levels.outputDbfs[static_cast<std::size_t>(channel)],db(std::abs(y)));levels.gainReductionDb[static_cast<std::size_t>(channel)]=s.moduleEnabled?reductionEnvelopeDb:0.0f;}}
    if(channels==1){levels.inputDbfs[1]=levels.inputDbfs[0];levels.outputDbfs[1]=levels.outputDbfs[0];levels.gainReductionDb[1]=levels.gainReductionDb[0];}return levels;
}
