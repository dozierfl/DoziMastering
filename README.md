# Dozi Mastering

Dozi Mastering is a standalone Apple Silicon macOS mastering application with analysis-guided processing, factory styles, streaming targets, original/mastered auditioning, conversational low-end revisions, project recovery, recent sessions, and verified WAV/MP3 export.

## Highlights

- Stereo WAV, AIFF, FLAC, and MP3 intake
- LUFS, RMS, peak, true-peak, crest-factor, stereo, and spectral analysis
- Deterministic mastering plans and reference matching
- Twelve factory styles, including Suno WAV Remaster
- Spotify delivery targets and custom LUFS/ceiling controls
- Original/mastered A/B with loudness matching
- Looping, stereo waveform navigation, monitor level, mute, and device selection
- Talk to Dozi low-end revisions with undo
- Project save/recovery and recent-session history
- Verified 32-bit float WAV and constant-bitrate MP3 export

## Build

Dependencies on macOS:

```sh
brew install cmake libsndfile libebur128 lame
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target DoziMastering -j 4
ctest --test-dir build-release --output-on-failure
```

JUCE 8.0.10 is fetched automatically unless `JUCE_SOURCE_DIR` points to a local checkout.

## Design

The presentation-refactor plan and interactive high-fidelity concept are in `docs/` and `design/`.
