# Phase 1 DSP and Integration Validation

## Automated reference tests

The suite verifies exact bypass, frame preservation, corrective-only linear-phase EQ, linked compression, mono-bass filtering, true-peak limiting, atomic export, reopen validation, and before/after analysis. True peak is independently remeasured with libebur128 after export.

## Real-mix integration run

Run on 2026-08-08 against four existing stereo music mixes. Source files were opened read-only. Derived 32-bit-float WAVs were written only under `/private/tmp`.

| Mix | Before LUFS | After LUFS | After dBTP | Frames |
|---|---:|---:|---:|---:|
| TwoStrikesInstrumental | -18.8333 | -18.7790 | -1.29214 | 5,537,280 |
| ThangsLikeDat Instrumental | -16.2972 | -17.8714 | -1.18219 | 9,829,008 |
| BlowYaWhistleRemixHipHop5.30 | -14.4948 | -16.1052 | -1.33553 | 9,247,073 |
| 1_IntrepidatiiFINAL Instrumental | -16.8090 | -16.1716 | -1.61080 | 11,527,740 |

All four exports completed, reopened, preserved frame count, and remained below the configured -1 dBTP hard ceiling. Loudness is best-effort: corrective EQ/compression can reduce integrated loudness, and the renderer never violates the peak ceiling merely to hit -14 LUFS.

The initial whole-file FFT convolution prototype was rejected during validation because its memory use scaled excessively with song length. It was replaced with bounded-memory 16,384-frame overlap-add convolution before the recorded runs.
