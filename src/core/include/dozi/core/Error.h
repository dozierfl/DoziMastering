#pragma once

#include <string>

namespace dozi::core {

enum class ErrorCode {
    unsupportedExtension,
    cannotOpenFile,
    invalidWaveHeader,
    unsupportedWaveFormat,
    emptyAudio,
    truncatedAudio,
    inconsistentWaveData,
    invalidConversion,
    conversionRefused,
    outputAlreadyExists,
    outputWriteFailed,
    invalidExportPlan,
    exportCancelled,
    exportValidationFailed
};

struct Error {
    ErrorCode code;
    std::string message;
};

} // namespace dozi::core
