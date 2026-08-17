#include "dozi/core/MasteringMp3Encoder.h"

#include <lame/lame.h>
#include <sndfile.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace dozi::core {
namespace {
constexpr std::array bitRates {32,40,48,56,64,80,96,112,128,160,192,224,256,320};
struct LameCloser { void operator()(lame_global_flags* value) const { if(value)lame_close(value); } };
struct SndCloser { void operator()(SNDFILE* value) const { if(value)sf_close(value); } };
struct FileCloser { void operator()(std::FILE* value) const { if(value)std::fclose(value); } };
int encodedBitRate(const std::filesystem::path&path){
    std::ifstream stream(path,std::ios::binary);std::array<unsigned char,65536>data{};stream.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(data.size()));const auto size=static_cast<std::size_t>(stream.gcount());
    for(std::size_t i=0;i+3<size;++i){const auto header=(static_cast<unsigned>(data[i])<<24)|(static_cast<unsigned>(data[i+1])<<16)|(static_cast<unsigned>(data[i+2])<<8)|data[i+3];if((header&0xffe00000u)!=0xffe00000u||(header&0x00180000u)!=0x00180000u||(header&0x00060000u)!=0x00020000u)continue;const auto index=(header>>12)&15u;if(index>0&&index<15)return bitRates[index-1];}
    return 0;
}
}

bool MasteringMp3Encoder::supportedBitRate(int value) noexcept
{
    return std::ranges::find(bitRates,value)!=bitRates.end();
}

Mp3EncodeResult MasteringMp3Encoder::encode(const std::filesystem::path&inputWav,
    const std::filesystem::path&outputMp3,int bitRateKbps,const CancellationToken&cancellation)const
{
    if(!supportedBitRate(bitRateKbps))throw std::invalid_argument("Unsupported MP3 bitrate");
    SF_INFO info{};std::unique_ptr<SNDFILE,SndCloser>input(sf_open(inputWav.c_str(),SFM_READ,&info));
    if(!input||info.channels!=2)throw std::runtime_error("MP3 export requires a readable stereo master");
    const auto temporary=std::filesystem::path(outputMp3.string()+".partial");
    std::filesystem::create_directories(outputMp3.parent_path());
    std::unique_ptr<std::FILE,FileCloser>output(std::fopen(temporary.c_str(),"wb"));
    if(!output)throw std::runtime_error("MP3 output could not be opened");
    std::unique_ptr<lame_global_flags,LameCloser>encoder(lame_init());if(!encoder)throw std::runtime_error("MP3 encoder initialization failed");
    const auto outputRate=std::min(info.samplerate,48000);lame_set_in_samplerate(encoder.get(),info.samplerate);lame_set_out_samplerate(encoder.get(),outputRate);
    lame_set_num_channels(encoder.get(),2);lame_set_mode(encoder.get(),JOINT_STEREO);lame_set_brate(encoder.get(),bitRateKbps);lame_set_VBR(encoder.get(),vbr_off);lame_set_quality(encoder.get(),0);
    if(lame_init_params(encoder.get())<0)throw std::runtime_error("MP3 encoder rejected the export settings");
    constexpr int framesPerBlock=4096;std::vector<float>pcm(framesPerBlock*2);std::vector<unsigned char>encoded(static_cast<std::size_t>(1.25*framesPerBlock+7200));
    for(sf_count_t frames;(frames=sf_readf_float(input.get(),pcm.data(),framesPerBlock))>0;){
        if(cancellation.isCancelled()){output.reset();std::filesystem::remove(temporary);throw std::runtime_error("Cancelled");}
        const auto bytes=lame_encode_buffer_interleaved_ieee_float(encoder.get(),pcm.data(),static_cast<int>(frames),encoded.data(),static_cast<int>(encoded.size()));
        if(bytes<0||std::fwrite(encoded.data(),1,static_cast<std::size_t>(bytes),output.get())!=static_cast<std::size_t>(bytes)){output.reset();std::filesystem::remove(temporary);throw std::runtime_error("MP3 encoding failed");}
    }
    const auto finalBytes=lame_encode_flush(encoder.get(),encoded.data(),static_cast<int>(encoded.size()));
    if(finalBytes<0||std::fwrite(encoded.data(),1,static_cast<std::size_t>(finalBytes),output.get())!=static_cast<std::size_t>(finalBytes)){output.reset();std::filesystem::remove(temporary);throw std::runtime_error("MP3 finalization failed");}
    output.reset();encoder.reset();input.reset();
    SF_INFO verification{};std::unique_ptr<SNDFILE,SndCloser>verified(sf_open(temporary.c_str(),SFM_READ,&verification));
    if(!verified||verification.channels!=2||verification.frames<=0||encodedBitRate(temporary)!=bitRateKbps){verified.reset();std::filesystem::remove(temporary);throw std::runtime_error("Encoded MP3 failed bitrate or audio verification");}
    verified.reset();std::error_code error;std::filesystem::rename(temporary,outputMp3,error);if(error){std::filesystem::remove(temporary);throw std::runtime_error("Atomic MP3 export commit failed");}
    return{outputMp3,bitRateKbps,outputRate};
}

} // namespace dozi::core
