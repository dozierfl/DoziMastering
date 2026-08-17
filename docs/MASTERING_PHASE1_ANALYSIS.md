# Dozi Mastering Phase 1: Audio I/O and Analysis

## Stack

Dozi Mastering uses C++20 inside the existing JUCE macOS desktop project. `libsndfile` provides one audited streaming decoder for WAV, AIFF, FLAC, and MP3, including lossless sources through 32-bit float / 192 kHz. MP3 is accepted as a lossy source with an explicit UI warning; it is decoded to floating point for processing and never used as the master export format. `libebur128` provides EBU R128 / ITU-R BS.1770 integrated, short-term, and momentary loudness plus oversampled true peak. The remaining measurements are implemented in the dependency-light `MasteringAnalyzer` core module so they can be tested without launching the UI or Pro Tools.

## Measurement definitions

- RMS is the square root of mean sample energy across both channels. Crest factor is sample peak minus RMS in dB.
- Noise floor is the 10th percentile of non-overlapping 100 ms block RMS values. Silence is represented by the explicit finite floor of -120 dBFS/LUFS.
- Clipping count is the number of decoded samples whose absolute value is at least 1.0. Inter-sample peak is reported when libebur128 true peak exceeds sample peak by more than 0.01 dB.
- Phase correlation is normalized L/R cross-correlation. Mid and side are `(L+R)/2` and `(L-R)/2`; their energy ratio is reported in dB.
- Mono compatibility loss compares the RMS of `(L+R)/2` with the original two-channel RMS. A negative value is level lost on summing to mono.
- Spectral analysis uses overlapping 4096-sample Hann-windowed FFTs. Centroid is the power-weighted mean frequency. Band values are measured power in sub (20–60 Hz), low (60–200), low-mid (200–500), mid (500–2000), high-mid (2–6 kHz), high (6–12 kHz), and air (12–20 kHz).
- A resonance is a local FFT maximum at least 4 dB above the mean power outside a two-bin guard region within a 16-bin neighborhood. Up to eight strongest peaks are returned with measured frequency and excess dB.

All values originate from decoded samples or libebur128. This module contains no classifications, inferred confidence values, decision rules, or mastering DSP.

## Build and test

```sh
brew install libsndfile libebur128
cmake -S . -B build
cmake --build build -j 8
ctest --test-dir build --output-on-failure
```
