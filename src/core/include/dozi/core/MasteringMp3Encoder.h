#pragma once

#include "dozi/core/CancellationToken.h"

#include <filesystem>
#include <string>

namespace dozi::core {

struct Mp3EncodeResult {
    std::filesystem::path output;
    int bitRateKbps = 0;
    int sampleRateHz = 0;
};

class MasteringMp3Encoder final {
public:
    [[nodiscard]] static bool supportedBitRate(int bitRateKbps) noexcept;
    [[nodiscard]] Mp3EncodeResult encode(const std::filesystem::path& inputWav,
        const std::filesystem::path& outputMp3, int bitRateKbps,
        const CancellationToken& cancellation = {}) const;
};

} // namespace dozi::core
