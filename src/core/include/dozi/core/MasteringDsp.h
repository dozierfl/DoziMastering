#pragma once
#include "dozi/core/MasteringStereoProcessor.h"
#include <vector>
#include <functional>
namespace dozi::core {
struct CorrectiveEqBand { double frequencyHz=1000, gainDb=0, q=4; };
struct LinearPhaseEqSettings { bool bypassed=false; std::vector<CorrectiveEqBand> bands; int taps=513; };
struct CompressorSettings {
    bool bypassed=false; double thresholdDbfs=-18, ratio=1.5, attackMs=30,
        releaseMs=150, makeupGainDb=0, sidechainHpfHz=0;
    bool automaticRelease=false;
};
struct LimiterSettings { bool bypassed=false; double inputGainDb=0, ceilingDbtp=-1, lookaheadMs=5, releaseMs=100; bool linked=true, truePeakEnabled=true; };
struct LimiterProcessingMetrics { double maximumGainReductionDb=0, finalSafetyTrimDb=0; };
struct DynamicEqBand {
    double frequencyHz=3000, q=1, thresholdDbfs=-24, maximumCutDb=3,
        attackMs=10, releaseMs=120;
    bool enabled=true, listen=false;
};
struct DynamicEqSettings { bool bypassed=false; std::vector<DynamicEqBand> bands; };
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
class LinearPhaseEq final { public: void process(StereoAudioBuffer&,const LinearPhaseEqSettings&,DspCancellation={}) const; };
class BroadbandCompressor final { public: void process(StereoAudioBuffer&,const CompressorSettings&,DspCancellation={}) const; };
class DynamicEqualizer final { public: void process(StereoAudioBuffer&,const DynamicEqSettings&,DspCancellation={}) const; };
class DeEsser final { public: void process(StereoAudioBuffer&,const DeEsserSettings&,DspCancellation={}) const; };
class MultibandCompressor final { public: void process(StereoAudioBuffer&,const MultibandCompressorSettings&,DspCancellation={}) const; };
class HarmonicSaturator final { public: void process(StereoAudioBuffer&,const SaturationSettings&,DspCancellation={}) const; };
class TruePeakLimiter final { public: [[nodiscard]] LimiterProcessingMetrics process(StereoAudioBuffer&,const LimiterSettings&,DspCancellation={}) const; [[nodiscard]] double truePeakDbtp(const StereoAudioBuffer&) const; };
}
