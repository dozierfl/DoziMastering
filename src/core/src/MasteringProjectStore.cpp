#include "dozi/core/MasteringProjectStore.h"
#include "dozi/core/MasteringVersionStore.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace dozi::core {
namespace {

void writeAnalysis(std::ostream& out,const std::optional<MasteringAnalysisResult>& value){
    out<<(value?1:0)<<'\n';if(!value)return;const auto&a=*value;
    out<<std::quoted(a.path.string())<<' '<<a.sampleRateHz<<' '<<a.channelCount<<' '<<a.sourceBitDepth<<' '<<a.frameCount<<' '
       <<std::setprecision(17)<<a.durationSeconds<<' '<<a.integratedLufs<<' '<<a.maximumShortTermLufs<<' '<<a.maximumMomentaryLufs<<' '
       <<a.samplePeakDbfs<<' '<<a.truePeakDbtp<<' '<<a.rmsDbfs<<' '<<a.crestFactorDb<<' '<<a.dcOffset<<' '<<a.noiseFloorDbfs<<' '
       <<a.clippedSampleCount<<' '<<a.interSamplePeakDetected<<' '<<a.phaseCorrelation<<' '<<a.midSideEnergyRatioDb<<' '
       <<a.monoCompatibilityLossDb<<' '<<a.spectralCentroidHz<<' '<<a.bands.size()<<' '<<a.resonances.size()<<'\n';
    for(const auto&b:a.bands)out<<std::quoted(b.name)<<' '<<b.lowerHz<<' '<<b.upperHz<<' '<<b.energyDb<<'\n';
    for(const auto&r:a.resonances)out<<r.frequencyHz<<' '<<r.excessDb<<'\n';
}

bool readAnalysis(std::istream& in,std::optional<MasteringAnalysisResult>& value){
    int present=0;if(!(in>>present))return false;if(!present){value.reset();return true;}MasteringAnalysisResult a;std::string path;std::size_t bands=0,resonances=0;
    if(!(in>>std::quoted(path)>>a.sampleRateHz>>a.channelCount>>a.sourceBitDepth>>a.frameCount>>a.durationSeconds>>a.integratedLufs
        >>a.maximumShortTermLufs>>a.maximumMomentaryLufs>>a.samplePeakDbfs>>a.truePeakDbtp>>a.rmsDbfs>>a.crestFactorDb
        >>a.dcOffset>>a.noiseFloorDbfs>>a.clippedSampleCount>>a.interSamplePeakDetected>>a.phaseCorrelation>>a.midSideEnergyRatioDb
        >>a.monoCompatibilityLossDb>>a.spectralCentroidHz>>bands>>resonances))return false;a.path=path;
    for(std::size_t i=0;i<bands;++i){SpectralBandMeasurement b;if(!(in>>std::quoted(b.name)>>b.lowerHz>>b.upperHz>>b.energyDb))return false;a.bands.push_back(std::move(b));}
    for(std::size_t i=0;i<resonances;++i){SpectralPeakMeasurement r;if(!(in>>r.frequencyHz>>r.excessDb))return false;a.resonances.push_back(r);}value=std::move(a);return true;
}

}

std::string MasteringProjectStore::save(const std::filesystem::path& path,const MasteringProject& project) const {
    if(project.sourcePath.empty())return "A project requires a source mix path.";
    const auto temporary=std::filesystem::path(path.string()+".partial");std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
    if(!out)return "Could not create project temporary file.";
    out<<"DOZI_MASTERING_PROJECT 3\n"<<std::quoted(project.sourcePath.string())<<' '<<std::quoted(project.referencePath.string())<<' '
       <<std::quoted(project.previewPath.string())<<' '<<std::quoted(project.exportPath.string())<<' '<<std::setprecision(17)
       <<project.playbackPositionSeconds<<' '<<project.playbackMastered<<' '<<project.loudnessMatched<<' '
       <<project.selectionStartSeconds<<' '<<project.selectionEndSeconds<<' '<<project.loopSelectionEnabled<<'\n';
    writeAnalysis(out,project.sourceAnalysis);writeAnalysis(out,project.referenceAnalysis);writeAnalysis(out,project.processedAnalysis);
    const auto planText=project.plan?MasteringVersionStore{}.serialize(*project.plan):std::string{};
    out<<(project.plan?1:0)<<' '<<planText.size()<<'\n';out.write(planText.data(),static_cast<std::streamsize>(planText.size()));out.close();
    if(!out){std::filesystem::remove(temporary);return "Could not finish project file.";}
    std::error_code error;std::filesystem::rename(temporary,path,error);if(error){std::filesystem::remove(temporary);return "Could not atomically commit project file.";}return {};
}

MasteringProjectOutcome MasteringProjectStore::open(const std::filesystem::path& path) const {
    std::ifstream in(path,std::ios::binary);std::string magic;int format=0;if(!(in>>magic>>format)||magic!="DOZI_MASTERING_PROJECT"||(format<1||format>3))return {{},"Unsupported mastering project format."};
    MasteringProject project;std::string source,reference,preview,exported;
    if(!(in>>std::quoted(source)>>std::quoted(reference)>>std::quoted(preview)>>std::quoted(exported)>>project.playbackPositionSeconds>>project.playbackMastered>>project.loudnessMatched))return {{},"Invalid project header."};
    if(format>=3&&!(in>>project.selectionStartSeconds>>project.selectionEndSeconds>>project.loopSelectionEnabled))return {{},"Invalid project loop selection."};
    project.sourcePath=source;project.referencePath=reference;project.previewPath=preview;project.exportPath=exported;
    if(!readAnalysis(in,project.sourceAnalysis))return {{},"Invalid saved analysis data."};
    if(format>=2&&!readAnalysis(in,project.referenceAnalysis))return {{},"Invalid saved reference analysis data."};
    if(!readAnalysis(in,project.processedAnalysis))return {{},"Invalid saved analysis data."};
    int hasPlan=0;std::size_t size=0;if(!(in>>hasPlan>>size))return {{},"Invalid saved plan header."};in.get();
    if(hasPlan){std::string text(size,'\0');in.read(text.data(),static_cast<std::streamsize>(size));if(!in)return {{},"Truncated saved plan."};auto restored=MasteringVersionStore{}.deserialize(text);if(!restored.plan)return {{},restored.error};project.plan=std::move(*restored.plan);}
    return {std::move(project),{}};
}

} // namespace dozi::core
