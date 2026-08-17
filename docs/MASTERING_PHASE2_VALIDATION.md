# Phase 2 validation

Validated 2026-08-09 on Apple Silicon.

## Automated validation

- Debug build completed.
- Release build completed.
- `ctest --test-dir build --output-on-failure`: 100% passed.
- Phase 2 planner test confirms the canonical eight-module chain.
- Target/version tests confirm sourced presets and lossless plan round-tripping.

## Real-mix validation

Reference: `1_ICanMakeYouFeelVideoShoot_(Instrumental).wav` (44.1 kHz stereo).

| Mix | Before | After | True peak | Frames | Modules |
|---|---:|---:|---:|---:|---:|
| `1_IntrepidatiiFINAL_(Instrumental).wav` | -16.809 LUFS | -16.543 LUFS | -3.1339 dBTP | 11,527,740 | 8 |
| `1_ICanMakeYouFeelVideoShoot_(Vocals).wav` | -20.3658 LUFS | -18.9631 LUFS | -4.97785 dBTP | 11,805,696 | 8 |

Both atomic exports reopened successfully, retained their source frame counts,
and remained below the configured -1 dBTP ceiling. The loudness target is best
effort: peak safety remains the hard constraint.

## UI integration

- Source and reference file choosers.
- Spotify Normal/Loud/Quiet plus Custom target selector.
- Eight type-addressed module toggles; no fixed numeric module assumptions.
- Saturation drive and wet/dry controls.
- Atomic Save Version, Restore Version, and Compare Version actions.
- Reference selection triggers fresh source/reference analysis.
- Mismatched reference sample rates and non-stereo references are blocked.

The Release app is installed at `/Applications/Dozi Mastering.app`.
The installed executable is arm64 and the complete bundle passes strict deep
code-signature verification with an ad-hoc development signature.
