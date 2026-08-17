#include "dozi/core/MasteringAnalyzer.h"

#include <ebur128.h>
#include <sndfile.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <memory>
#include <numbers>
#include <numeric>
#include <tuple>

namespace dozi::core {
namespace {

constexpr double floorDb = -120.0;
constexpr std::size_t fftSize = 4096;
constexpr std::size_t hopSize = fftSize / 2;

double db(double amplitude) { return amplitude <= 1.0e-12 ? floorDb : 20.0 * std::log10(amplitude); }
double powerDb(double power) { return power <= 1.0e-12 ? floorDb : 10.0 * std::log10(power); }

void fft(std::vector<std::complex<double>>& values)
{
    for (std::size_t i = 1, j = 0; i < values.size(); ++i) {
        auto bit = values.size() >> 1U;
        for (; j & bit; bit >>= 1U) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(values[i], values[j]);
    }
    for (std::size_t length = 2; length <= values.size(); length <<= 1U) {
        const auto angle = -2.0 * std::numbers::pi / static_cast<double>(length);
        const std::complex<double> root(std::cos(angle), std::sin(angle));
        for (std::size_t start = 0; start < values.size(); start += length) {
            std::complex<double> factor(1.0, 0.0);
            for (std::size_t offset = 0; offset < length / 2; ++offset) {
                const auto even = values[start + offset];
                const auto odd = factor * values[start + offset + length / 2];
                values[start + offset] = even + odd;
                values[start + offset + length / 2] = even - odd;
                factor *= root;
            }
        }
    }
}

struct Spectrum {
    explicit Spectrum(int rate) : sampleRate(rate), power(fftSize / 2 + 1) {}
    int sampleRate;
    std::vector<double> pending;
    std::vector<double> power;
    std::uint64_t windows = 0;

    void add(double mono)
    {
        pending.push_back(mono);
        if (pending.size() < fftSize) return;
        std::vector<std::complex<double>> values(fftSize);
        for (std::size_t i = 0; i < fftSize; ++i) {
            const auto window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * i / (fftSize - 1));
            values[i] = pending[i] * window;
        }
        fft(values);
        for (std::size_t i = 0; i < power.size(); ++i) power[i] += std::norm(values[i]);
        ++windows;
        pending.erase(pending.begin(), pending.begin() + static_cast<std::ptrdiff_t>(hopSize));
    }
};

std::vector<SpectralBandMeasurement> bands(const Spectrum& spectrum)
{
    static constexpr std::array definitions {
        std::tuple{"sub", 20.0, 60.0}, std::tuple{"low", 60.0, 200.0},
        std::tuple{"low-mid", 200.0, 500.0}, std::tuple{"mid", 500.0, 2000.0},
        std::tuple{"high-mid", 2000.0, 6000.0}, std::tuple{"high", 6000.0, 12000.0},
        std::tuple{"air", 12000.0, 20000.0}
    };
    std::vector<SpectralBandMeasurement> result;
    for (const auto& [name, low, high] : definitions) {
        double energy = 0.0;
        for (std::size_t bin = 1; bin < spectrum.power.size(); ++bin) {
            const auto hz = static_cast<double>(bin) * spectrum.sampleRate / fftSize;
            if (hz >= low && hz < high) energy += spectrum.power[bin];
        }
        result.push_back({name, low, high, powerDb(energy / std::max<std::uint64_t>(1, spectrum.windows))});
    }
    return result;
}

std::vector<SpectralPeakMeasurement> resonances(const Spectrum& spectrum)
{
    std::vector<SpectralPeakMeasurement> result;
    if (spectrum.windows == 0) return result;
    constexpr std::size_t radius = 16;
    constexpr std::size_t guard = 2;
    for (std::size_t bin = radius; bin + radius < spectrum.power.size(); ++bin) {
        const auto hz = static_cast<double>(bin) * spectrum.sampleRate / fftSize;
        if (hz < 80.0 || hz > 18000.0) continue;
        double local = 0.0;
        std::size_t localBins = 0;
        for (std::size_t neighbour = bin - radius; neighbour <= bin + radius; ++neighbour) {
            const auto distance = neighbour > bin ? neighbour - bin : bin - neighbour;
            if (distance > guard) { local += spectrum.power[neighbour]; ++localBins; }
        }
        local /= static_cast<double>(localBins);
        const auto excess = powerDb(spectrum.power[bin]) - powerDb(local);
        if (excess >= 4.0 && spectrum.power[bin] >= spectrum.power[bin - 1]
            && spectrum.power[bin] >= spectrum.power[bin + 1]) result.push_back({hz, excess});
    }
    std::ranges::sort(result, std::greater{}, &SpectralPeakMeasurement::excessDb);
    if (result.size() > 8) result.resize(8);
    return result;
}

struct SndCloser { void operator()(SNDFILE* file) const { if (file) sf_close(file); } };
struct EburCloser { void operator()(ebur128_state* state) const { if (state) ebur128_destroy(&state); } };

} // namespace

MasteringAnalysisOutcome MasteringAnalyzer::analyse(
    const std::filesystem::path& path, const CancellationToken& cancellation) const
{
    SF_INFO info {};
    std::unique_ptr<SNDFILE, SndCloser> file(sf_open(path.c_str(), SFM_READ, &info));
    if (!file) return {{}, Error{ErrorCode::cannotOpenFile, sf_strerror(nullptr)}};
    const auto container = info.format & SF_FORMAT_TYPEMASK;
    const auto isMpeg = container == SF_FORMAT_MPEG;
    if (container != SF_FORMAT_WAV && container != SF_FORMAT_AIFF && container != SF_FORMAT_FLAC && !isMpeg)
        return {{}, Error{ErrorCode::unsupportedExtension, "Only WAV, AIFF, FLAC, and MP3 are supported."}};
    const auto subtype = info.format & SF_FORMAT_SUBMASK;
    const auto isSupportedMpeg = isMpeg && (subtype == SF_FORMAT_MPEG_LAYER_I
        || subtype == SF_FORMAT_MPEG_LAYER_II || subtype == SF_FORMAT_MPEG_LAYER_III);
    if (!isSupportedMpeg && subtype != SF_FORMAT_PCM_16 && subtype != SF_FORMAT_PCM_24
        && subtype != SF_FORMAT_PCM_32 && subtype != SF_FORMAT_FLOAT)
        return {{}, Error{ErrorCode::unsupportedWaveFormat,
            "Supported encodings are MP3, 16-, 24-, or 32-bit PCM, and 32-bit float."}};
    if (info.channels != 2 || info.samplerate <= 0 || info.samplerate > 192000 || info.frames <= 0)
        return {{}, Error{ErrorCode::unsupportedWaveFormat, "Mastering input must be non-empty stereo audio at no more than 192 kHz."}};

    constexpr int mode = EBUR128_MODE_I | EBUR128_MODE_S | EBUR128_MODE_M | EBUR128_MODE_TRUE_PEAK;
    std::unique_ptr<ebur128_state, EburCloser> loudness(ebur128_init(2, static_cast<unsigned long>(info.samplerate), mode));
    if (!loudness) return {{}, Error{ErrorCode::cannotOpenFile, "libebur128 could not initialize."}};

    MasteringAnalysisResult result;
    result.path = path; result.sampleRateHz = info.samplerate; result.channelCount = info.channels;
    result.frameCount = static_cast<std::uint64_t>(info.frames);
    result.durationSeconds = static_cast<double>(info.frames) / info.samplerate;
    result.sourceBitDepth = isMpeg ? 0 : subtype == SF_FORMAT_PCM_16 ? 16 : subtype == SF_FORMAT_PCM_24 ? 24 : 32;

    Spectrum spectrum(info.samplerate);
    std::vector<double> samples(4096 * 2);
    std::vector<double> blockRms;
    const auto noiseBlockFrames = std::max(1, info.samplerate / 10);
    double sumSquares = 0.0, sum = 0.0, peak = 0.0, left2 = 0.0, right2 = 0.0, cross = 0.0;
    double mid2 = 0.0, side2 = 0.0, noiseEnergy = 0.0;
    int noiseFrames = 0;
    while (const auto frames = sf_readf_double(file.get(), samples.data(), 4096)) {
        if (cancellation.isCancelled()) return {};
        if (ebur128_add_frames_double(loudness.get(), samples.data(), static_cast<std::size_t>(frames)) != EBUR128_SUCCESS)
            return {{}, Error{ErrorCode::inconsistentWaveData, "libebur128 rejected decoded samples."}};
        double current = 0.0;
        if (ebur128_loudness_momentary(loudness.get(), &current) == EBUR128_SUCCESS && std::isfinite(current))
            result.maximumMomentaryLufs = std::max(result.maximumMomentaryLufs, current);
        if (ebur128_loudness_shortterm(loudness.get(), &current) == EBUR128_SUCCESS && std::isfinite(current))
            result.maximumShortTermLufs = std::max(result.maximumShortTermLufs, current);
        for (sf_count_t frame = 0; frame < frames; ++frame) {
            const auto left = samples[static_cast<std::size_t>(frame) * 2];
            const auto right = samples[static_cast<std::size_t>(frame) * 2 + 1];
            if (!std::isfinite(left) || !std::isfinite(right))
                return {{}, Error{ErrorCode::inconsistentWaveData, "Decoded audio contains a non-finite sample."}};
            for (const auto sample : {left, right}) {
                sumSquares += sample * sample; sum += sample; peak = std::max(peak, std::abs(sample));
                if (std::abs(sample) >= 1.0) ++result.clippedSampleCount;
            }
            left2 += left * left; right2 += right * right; cross += left * right;
            const auto mid = (left + right) * 0.5; const auto side = (left - right) * 0.5;
            mid2 += mid * mid; side2 += side * side; spectrum.add(mid);
            noiseEnergy += (left * left + right * right) * 0.5; ++noiseFrames;
            if (noiseFrames == noiseBlockFrames) {
                blockRms.push_back(std::sqrt(noiseEnergy / noiseFrames)); noiseEnergy = 0.0; noiseFrames = 0;
            }
        }
    }
    if (noiseFrames > 0) blockRms.push_back(std::sqrt(noiseEnergy / noiseFrames));
    if (cancellation.isCancelled()) return {};

    const auto sampleCount = static_cast<double>(result.frameCount * 2);
    result.samplePeakDbfs = db(peak); result.rmsDbfs = db(std::sqrt(sumSquares / sampleCount));
    result.crestFactorDb = result.samplePeakDbfs - result.rmsDbfs; result.dcOffset = sum / sampleCount;
    std::ranges::sort(blockRms);
    if (!blockRms.empty()) result.noiseFloorDbfs = db(blockRms[static_cast<std::size_t>(0.1 * (blockRms.size() - 1))]);
    const auto denominator = std::sqrt(left2 * right2);
    result.phaseCorrelation = denominator <= 1.0e-20 ? 0.0 : std::clamp(cross / denominator, -1.0, 1.0);
    result.midSideEnergyRatioDb = powerDb(mid2) - powerDb(side2);
    const auto stereoRms = std::sqrt((left2 + right2) / sampleCount);
    const auto monoRms = std::sqrt(mid2 / result.frameCount);
    result.monoCompatibilityLossDb = db(monoRms) - db(stereoRms);

    if (ebur128_loudness_global(loudness.get(), &result.integratedLufs) != EBUR128_SUCCESS
        || !std::isfinite(result.integratedLufs)) result.integratedLufs = floorDb;
    for (unsigned channel = 0; channel < 2; ++channel) {
        double channelPeak = 0.0;
        if (ebur128_true_peak(loudness.get(), channel, &channelPeak) == EBUR128_SUCCESS)
            result.truePeakDbtp = std::max(result.truePeakDbtp, db(channelPeak));
    }
    result.interSamplePeakDetected = result.truePeakDbtp > result.samplePeakDbfs + 0.01;

    double weightedHz = 0.0, spectralPower = 0.0;
    for (std::size_t bin = 1; bin < spectrum.power.size(); ++bin) {
        const auto hz = static_cast<double>(bin) * info.samplerate / fftSize;
        weightedHz += hz * spectrum.power[bin]; spectralPower += spectrum.power[bin];
    }
    result.spectralCentroidHz = spectralPower <= 1.0e-20 ? 0.0 : weightedHz / spectralPower;
    result.bands = bands(spectrum); result.resonances = resonances(spectrum);
    return {std::move(result), {}};
}

} // namespace dozi::core
