#include "dozi/core/MasteringTargets.h"

#include <algorithm>

namespace dozi::core {

const std::vector<MasteringTargetPreset>& verifiedMasteringTargets() {
    static const std::vector<MasteringTargetPreset> targets {
        {"spotify-normal", "Spotify Normal", -14.0, -1.0,
            "https://support.spotify.com/us/artists/article/loudness-normalization/",
            "Spotify states -14 LUFS Normal and recommends masters at -14 LUFS with true peak below -1 dBTP."},
        {"spotify-loud", "Spotify Loud", -11.0, -2.0,
            "https://support.spotify.com/us/artists/article/loudness-normalization/",
            "Spotify states -11 LUFS Loud and recommends true peak below -2 dBTP for masters louder than -14 LUFS."},
        {"spotify-quiet", "Spotify Quiet", -19.0, -1.0,
            "https://support.spotify.com/us/artists/article/loudness-normalization/",
            "Spotify states -19 LUFS Quiet; Dozi retains Spotify's published -1 dBTP mastering ceiling."}
    };
    return targets;
}

std::optional<MasteringTargetPreset> masteringTarget(std::string_view id) {
    const auto& targets=verifiedMasteringTargets();
    const auto found=std::find_if(targets.begin(),targets.end(),[id](const auto& target){return target.id==id;});
    return found==targets.end()?std::nullopt:std::optional<MasteringTargetPreset>(*found);
}

MasteringDecisionConfig decisionConfig(const MasteringTargetPreset& target) {
    MasteringDecisionConfig config;
    config.targetId=target.id;
    config.targetIntegratedLufs=target.integratedLufs;
    config.truePeakCeilingDbtp=target.truePeakCeilingDbtp;
    return config;
}

} // namespace dozi::core
