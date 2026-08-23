#pragma once
#include "dozi/core/MasteringStereoProcessor.h"
#include <vector>
#include <functional>
#include <array>
namespace dozi::core {
struct CorrectiveEqBand { double frequencyHz=1000, gainDb=0, q=4; };
struct LinearPhaseEqSettings { bool bypassed=false; std::vector<CorrectiveEqBand> bands; int taps=513; };
struct CompressorSettings {
    bool bypassed=false; double thresholdDbfs=-18, ratio=1.5, attackMs=30,
        releaseMs=150, makeupGainDb=0, sidechainHpfHz=0;
    bool automaticRelease=false;
    bool automaticMakeup=false;
};
struct LimiterSettings { bool bypassed=false; double inputGainDb=0, ceilingDbtp=-1, lookaheadMs=5, releaseMs=100; bool linked=true, truePeakEnabled=true; };
struct LimiterProcessingMetrics { double maximumGainReductionDb=0, finalSafetyTrimDb=0; };
struct DynamicEqBand {
    double frequencyHz=3000, q=1, thresholdDbfs=-24, maximumCutDb=3,
        attackMs=10, releaseMs=120;
    bool enabled=true, listen=false;
};
struct DynamicEqSettings { bool bypassed=false; std::vector<DynamicEqBand> bands; };
struct DynamicBandState { double left1=0,left2=0,right1=0,right2=0,envelope=0; };
struct DeEsserSettings {
    bool bypassed=false; double frequencyHz=6500, q=1, thresholdDbfs=-30,
        ratio=3, maximumReductionDb=6, attackMs=2, releaseMs=80;
    bool listen=false;
};
struct MultibandCompressorBandSettings {
    double thresholdDbfs=-18, ratio=1.5, attackMs=20, releaseMs=150,
        maximumReductionDb=3;
    bool solo=false;
};
struct MultibandCompressorSettings {
    bool bypassed=false;
    double lowCrossoverHz=120, midCrossoverHz=700, highCrossoverHz=4000;
    MultibandCompressorBandSettings low, lowMid, highMid, high;
};
enum class SaturationMode { clean=0, warm=1, rich=2, tape=warm, tube=rich };
struct SaturationSettings {
    bool bypassed=false;
    SaturationMode mode=SaturationMode::warm;
    double driveDb=2, mix=1, outputTrimDb=0;
    int oversamplingFactor=4;
    bool automaticLevelCompensation=true;
    double maximumCompensationDb=3;
    double outputCeilingDbfs=-0.1;
};
using DspCancellation = std::function<bool()>;
class LinearPhaseEq final {
public:
    void reset() noexcept;
    void process(StereoAudioBuffer&,const LinearPhaseEqSettings&,DspCancellation={});
private:
    struct BandState { double left1=0,left2=0,right1=0,right2=0; };
    std::vector<BandState> states;
};
class BroadbandCompressor final { public: void reset() noexcept; void process(StereoAudioBuffer&,const CompressorSettings&,DspCancellation={}); private: double gain=1,lowL=0,lowR=0; };
class DynamicEqualizer final { public: void reset() noexcept; void process(StereoAudioBuffer&,const DynamicEqSettings&,DspCancellation={}); private: std::vector<DynamicBandState> states; };
class DeEsser final { public: void reset() noexcept; void process(StereoAudioBuffer&,const DeEsserSettings&,DspCancellation={}); private: DynamicBandState state; };
class MultibandCompressor final { public: void reset() noexcept; void process(StereoAudioBuffer&,const MultibandCompressorSettings&,DspCancellation={}); private: std::array<double,6> crossoverStates{};std::array<double,4> gains{1,1,1,1};std::array<std::vector<double>,8> scratch; };
class HarmonicSaturator final { public: void reset() noexcept; void process(StereoAudioBuffer&,const SaturationSettings&,DspCancellation={}); private: double previousLeft=0,previousRight=0,inputPower=0,outputPower=0,dcLeft=0,dcRight=0; };
class TruePeakLimiter final { public: void reset() noexcept; [[nodiscard]] LimiterProcessingMetrics process(StereoAudioBuffer&,const LimiterSettings&,DspCancellation={}); [[nodiscard]] double truePeakDbtp(const StereoAudioBuffer&) const; private: double linkedGain=1,leftGain=1,rightGain=1; };
}
