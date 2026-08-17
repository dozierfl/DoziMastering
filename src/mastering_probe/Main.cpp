#include "dozi/core/MasteringAnalyzer.h"
#include "dozi/core/MasteringDecisionEngine.h"
#include "dozi/core/MasteringReference.h"
#include "dozi/core/MasteringRenderer.h"

#include <filesystem>
#include <iostream>

int main(int argc,char**argv){
    bool phase2=argc>=5&&std::string_view(argv[1])=="--phase2";
    if((!phase2&&argc<3)||(phase2&&argc<5)){
        std::cerr<<"usage: dozi_mastering_probe OUTPUT_DIR MIX...\n"
                 <<"   or: dozi_mastering_probe --phase2 REFERENCE OUTPUT_DIR MIX...\n";return 2;
    }
    const std::filesystem::path outputDirectory=argv[phase2?3:1];
    std::optional<dozi::core::MasteringAnalysisResult> reference;
    dozi::core::MasteringAnalyzer analyzer;
    if(phase2){const auto measured=analyzer.analyse(argv[2]);if(!measured.result){std::cerr<<"reference: "<<(measured.error?measured.error->message:"cancelled")<<'\n';return 1;}reference=*measured.result;}
    std::filesystem::create_directories(outputDirectory);
    for(int i=phase2?4:2;i<argc;++i){
        const std::filesystem::path source=argv[i];const auto measured=analyzer.analyse(source);
        if(!measured.result){std::cerr<<source<<": "<<(measured.error?measured.error->message:"cancelled")<<'\n';return 1;}
        auto plan=phase2?dozi::core::MasteringReferenceMatcher{}.createPhase2Plan(*measured.result,reference)
                        :dozi::core::MasteringDecisionEngine{}.createPlan(*measured.result);
        if(phase2)for(auto& module:plan.modules)if(module.type==dozi::core::MasteringModuleType::deEsser||module.type==dozi::core::MasteringModuleType::saturation)module.bypassed=false;
        const auto output=outputDirectory/(source.stem().string()+"-dozi.wav");
        try{const auto rendered=dozi::core::MasteringRenderer{}.render(source,output,plan);std::cout<<source.filename()<<": "<<rendered.before.integratedLufs<<" -> "<<rendered.after.integratedLufs<<" LUFS, "<<rendered.after.truePeakDbtp<<" dBTP, "<<rendered.after.frameCount<<" frames, "<<plan.modules.size()<<" modules\n";}
        catch(const std::exception&e){std::cerr<<source<<": "<<e.what()<<'\n';return 1;}
    }
    return 0;
}
