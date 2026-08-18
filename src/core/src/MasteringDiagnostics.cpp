#include "dozi/core/MasteringDiagnostics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <iomanip>
#include <sstream>

namespace dozi::core {
namespace {

struct BandRule { const char* id; const char* excess; const char* deficient; const char* family; const char* excessAction; const char* deficientAction; };
constexpr std::array rules {
    BandRule{"sub","Too much sub / rumble","Weak bass","Tonal balance","Check for non-musical rumble; consider a gentle 20-30 Hz high-pass filter or low shelf.","If the style calls for it, consider a restrained low-shelf lift."},
    BandRule{"low","Bass-heavy","Thin","Tonal balance","Consider a broad low-shelf reduction; use dynamic EQ if the excess is intermittent.","Consider a gentle low or low-mid lift after checking headroom."},
    BandRule{"low-mid","Muddy / cloudy","Hollow","Tonal balance","Inspect 200-500 Hz and consider a broad or dynamic reduction at the measured excess.","Consider a broad, restrained body lift if the reference relationship is intentional."},
    BandRule{"mid","Honky / congested","Hollow","Tonal balance","Inspect 500 Hz-2 kHz for a resonance; prefer a narrow or dynamic reduction when localized.","Consider a broad mid-body lift; verify that this is not an arrangement issue."},
    BandRule{"high-mid","Harsh / fatiguing","Needs presence","Tonal balance","Inspect 2-6 kHz; dynamic EQ is safer than a permanent broad cut when the excess is event-dependent.","Consider a small broad presence lift while checking vocal balance."},
    BandRule{"high","Too bright / brittle","Dull / veiled","Tonal balance","Consider a gentle or dynamic high-frequency reduction.","Consider a restrained high shelf after checking noise and sibilance."},
    BandRule{"air","Too airy","No air","Tonal balance","Consider a very gentle air-band reduction and verify that the excess is not hiss.","Consider a very gentle air shelf if the source noise floor permits."}
};

double strength(double amount, double threshold) { return std::clamp((amount-threshold)/6.0+0.5,0.0,1.0); }
std::string number(double value,int precision=2) { std::ostringstream out;out<<std::fixed<<std::setprecision(precision)<<value;return out.str(); }
std::string measured(double value) { return number(value)+" dB"; }

} // namespace

std::vector<MasteringDiagnostic> MasteringDiagnosticEngine::diagnose(const MasteringAnalysisResult& source,const std::optional<MasteringAnalysisResult>& reference,const MasteringDiagnosticConfig& config) const {
    std::vector<MasteringDiagnostic> findings;
    if(reference&&source.bands.size()==reference->bands.size()&&!source.bands.empty()){
        std::vector<double> offsets;offsets.reserve(source.bands.size());
        for(std::size_t i=0;i<source.bands.size();++i)offsets.push_back(source.bands[i].energyDb-reference->bands[i].energyDb);
        std::ranges::sort(offsets);const auto levelOffset=offsets[offsets.size()/2];
        for(const auto&rule:rules){const auto sourceBand=std::ranges::find(source.bands,std::string(rule.id),&SpectralBandMeasurement::name);const auto referenceBand=std::ranges::find(reference->bands,std::string(rule.id),&SpectralBandMeasurement::name);if(sourceBand==source.bands.end()||referenceBand==reference->bands.end())continue;const auto delta=(sourceBand->energyDb-referenceBand->energyDb)-levelOffset;if(std::abs(delta)<config.referenceBandDeviationDb)continue;const auto excess=delta>0;const auto resonance=std::ranges::find_if(source.resonances,[&](const auto&peak){return peak.frequencyHz>=sourceBand->lowerHz&&peak.frequencyHz<sourceBand->upperHz&&peak.excessDb>=config.resonanceExcessDb;});const auto dynamic=resonance!=source.resonances.end();findings.push_back({std::string("reference-")+rule.id+(excess?"-excess":"-deficit"),excess?rule.excess:rule.deficient,rule.family,excess?"This range is elevated relative to the selected reference.":"This range is recessed relative to the selected reference.",std::string(rule.id)+" band is "+measured(delta)+(excess?" above":" below")+" the level-matched reference relationship.",excess?rule.excessAction:rule.deficientAction,DiagnosticBasis::referenceComparison,strength(std::abs(delta),config.referenceBandDeviationDb),sourceBand->lowerHz,sourceBand->upperHz,dynamic,false});}
    }
    for(const auto&peak:source.resonances){if(peak.excessDb<config.resonanceExcessDb)continue;const char*descriptor=peak.frequencyHz<160?"Possible boom":peak.frequencyHz<500?"Possible boxiness / mud":peak.frequencyHz<1500?"Possible honk / nasal resonance":peak.frequencyHz<6000?"Possible harsh or piercing resonance":"Possible brittle or sibilant resonance";findings.push_back({"resonance",descriptor,"Tonal balance","A localized spectral peak may be perceptually prominent.","Peak at "+number(peak.frequencyHz,0)+" Hz is "+number(peak.excessDb)+" dB above its local neighborhood.","Audition a dynamic EQ centered near "+number(peak.frequencyHz,0)+" Hz with bounded reduction; do not cut automatically.",DiagnosticBasis::objectiveMeasurement,strength(peak.excessDb,config.resonanceExcessDb),peak.frequencyHz,peak.frequencyHz,true,false});}
    if(source.clippedSampleCount>0)findings.push_back({"clipping","Distorted / clipped","Dynamics","The source contains samples at or above full scale.",std::to_string(source.clippedSampleCount)+" clipped samples detected.","Reduce upstream level or return to the mix before further limiting.",DiagnosticBasis::objectiveMeasurement,1.0,0,0,false,true});
    if(source.crestFactorDb<config.lowCrestFactorDb)findings.push_back({"low-crest","Squashed / over-compressed","Dynamics","Low crest factor indicates limited peak-to-average contrast.","Crest factor is "+number(source.crestFactorDb)+" dB.","Avoid additional compression or limiting; compare with the unmastered mix and consider returning to the mix.",DiagnosticBasis::objectiveMeasurement,strength(config.lowCrestFactorDb-source.crestFactorDb,0.0),0,0,false,true});
    else if(source.crestFactorDb>config.highCrestFactorDb)findings.push_back({"high-crest","Peaky / very dynamic","Dynamics","High crest factor indicates large transient-to-average contrast.","Crest factor is "+number(source.crestFactorDb)+" dB.","Audition gentle peak control or bus compression while preserving transient impact.",DiagnosticBasis::objectiveMeasurement,strength(source.crestFactorDb-config.highCrestFactorDb,0.0),0,0,false,false});
    if(source.phaseCorrelation<config.unsafeCorrelation)findings.push_back({"phase","Phasey / too wide","Stereo / phase","Low correlation indicates possible mono-compatibility risk.","L/R phase correlation is "+number(source.phaseCorrelation)+"; mono level change is "+number(source.monoCompatibilityLossDb)+" dB.","Reduce stereo widening, inspect polarity, and audition in mono before export.",DiagnosticBasis::objectiveMeasurement,strength(config.unsafeCorrelation-source.phaseCorrelation,0.0),0,0,false,false});
    std::ranges::stable_sort(findings,[](const auto&a,const auto&b){return a.strength>b.strength;});if(findings.size()>10)findings.resize(10);return findings;
}

} // namespace dozi::core
