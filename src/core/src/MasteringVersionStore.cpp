#include "dozi/core/MasteringVersionStore.h"

#include <charconv>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>

namespace dozi::core {
namespace {
std::string quoted(const std::string& value){std::ostringstream stream;stream<<std::quoted(value);return stream.str();}
}

std::string MasteringVersionStore::save(const std::filesystem::path& path,const MasteringPlan& plan) const {
    const auto temporary=std::filesystem::path(path.string()+".partial");
    std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
    if(!out)return "Could not create mastering version temporary file.";
    out<<serialize(plan);
    out.close();if(!out){std::filesystem::remove(temporary);return "Could not finish mastering version file.";}
    std::error_code error;std::filesystem::rename(temporary,path,error);
    if(error){std::filesystem::remove(temporary);return "Could not atomically commit mastering version file.";}
    return {};
}

MasteringVersionOutcome MasteringVersionStore::restore(const std::filesystem::path& path) const {
    std::ifstream in(path,std::ios::binary);
    if(!in)return { {}, "Could not open mastering version file." };
    std::ostringstream contents;contents<<in.rdbuf();return deserialize(contents.str());
}

std::string MasteringVersionStore::serialize(const MasteringPlan& plan) const {
    std::ostringstream out;
    out<<"DOZI_MASTERING_VERSION 2\n"<<plan.schemaVersion<<' '<<quoted(plan.targetId)<<' '<<quoted(plan.styleId)<<' '
       <<std::setprecision(17)<<plan.targetIntegratedLufs<<' '<<plan.truePeakCeilingDbtp<<' '<<plan.modules.size()<<'\n';
    for(const auto& module:plan.modules){
        out<<static_cast<int>(module.type)<<' '<<module.bypassed<<' '<<module.parameters.size()<<' '<<module.reasons.size()<<'\n';
        for(const auto&[key,value]:module.parameters)out<<quoted(key)<<' '<<std::setprecision(17)<<value<<'\n';
        for(const auto& reason:module.reasons)out<<quoted(reason.measurement)<<' '<<std::setprecision(17)<<reason.measuredValue<<' '
            <<quoted(reason.unit)<<' '<<quoted(reason.ruleId)<<' '<<quoted(reason.ruleExpression)<<' '<<quoted(reason.action)<<'\n';
    }
    return out.str();
}

MasteringVersionOutcome MasteringVersionStore::deserialize(const std::string& data) const {
    std::istringstream in(data);std::string magic;int format=0;
    if(!(in>>magic>>format)||magic!="DOZI_MASTERING_VERSION"||(format!=1&&format!=2))return {{},"Unsupported mastering version format."};
    MasteringPlan plan;std::size_t moduleCount=0;
    in>>plan.schemaVersion>>std::quoted(plan.targetId);if(format>=2)in>>std::quoted(plan.styleId);else plan.styleId="balanced-streaming";in>>plan.targetIntegratedLufs>>plan.truePeakCeilingDbtp>>moduleCount;
    if(!in||plan.schemaVersion!=2)return {{},"Invalid or unsupported mastering plan schema."};
    for(std::size_t index=0;index<moduleCount;++index){
        int type=0,bypassed=0;std::size_t parameterCount=0,reasonCount=0;
        if(!(in>>type>>bypassed>>parameterCount>>reasonCount)||type<0||type>static_cast<int>(MasteringModuleType::truePeakLimiter))
            return {{},"Invalid mastering module record."};
        MasteringModulePlan module{static_cast<MasteringModuleType>(type),bypassed!=0,{},{}};
        for(std::size_t p=0;p<parameterCount;++p){std::string key;double value=0;if(!(in>>std::quoted(key)>>value))return {{},"Invalid mastering parameter record."};module.parameters[key]=value;}
        for(std::size_t r=0;r<reasonCount;++r){MasteringDecisionTrace reason;if(!(in>>std::quoted(reason.measurement)>>reason.measuredValue>>std::quoted(reason.unit)>>std::quoted(reason.ruleId)>>std::quoted(reason.ruleExpression)>>std::quoted(reason.action)))return {{},"Invalid mastering decision trace."};module.reasons.push_back(std::move(reason));}
        plan.modules.push_back(std::move(module));
    }
    return {std::move(plan),{}};
}

std::vector<MasteringVersionDifference> MasteringVersionStore::compare(const MasteringPlan& left,const MasteringPlan& right) const {
    std::vector<MasteringVersionDifference> result;
    if(left.targetId!=right.targetId)result.push_back({"targetId",{},{},left.targetId,right.targetId});
    if(left.styleId!=right.styleId)result.push_back({"styleId",{},{},left.styleId,right.styleId});
    if(left.targetIntegratedLufs!=right.targetIntegratedLufs)result.push_back({"targetIntegratedLufs",left.targetIntegratedLufs,right.targetIntegratedLufs,{},{}});
    if(left.truePeakCeilingDbtp!=right.truePeakCeilingDbtp)result.push_back({"truePeakCeilingDbtp",left.truePeakCeilingDbtp,right.truePeakCeilingDbtp,{},{}});
    const auto count=std::max(left.modules.size(),right.modules.size());
    for(std::size_t i=0;i<count;++i){
        if(i>=left.modules.size()||i>=right.modules.size()){result.push_back({"modules."+std::to_string(i),{}, {},{},{}});continue;}
        if(left.modules[i].type!=right.modules[i].type)result.push_back({"modules."+std::to_string(i)+".type",static_cast<double>(left.modules[i].type),static_cast<double>(right.modules[i].type),{},{}});
        if(left.modules[i].bypassed!=right.modules[i].bypassed)result.push_back({"modules."+std::to_string(i)+".bypassed",left.modules[i].bypassed?1.0:0.0,right.modules[i].bypassed?1.0:0.0,{},{}});
        for(const auto&[key,value]:left.modules[i].parameters){const auto found=right.modules[i].parameters.find(key);if(found==right.modules[i].parameters.end()||found->second!=value)result.push_back({"modules."+std::to_string(i)+"."+key,value,found==right.modules[i].parameters.end()?std::optional<double>{}:std::optional<double>{found->second},{},{}});}
        for(const auto&[key,value]:right.modules[i].parameters)if(!left.modules[i].parameters.contains(key))result.push_back({"modules."+std::to_string(i)+"."+key,{},value,{},{}});
    }
    return result;
}

} // namespace dozi::core
