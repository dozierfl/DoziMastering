# Dozi Mastering Phase 1: Decision Engine

The decision engine is deterministic. It consumes only `MasteringAnalysisResult` and an explicit `MasteringDecisionConfig`; it does not classify genre, infer intent, invent confidence, or inspect filenames.

Every enabled adjustment contains a trace with the measured value and unit, rule identifier and expression, and proposed action. The ordered plan is linear-phase corrective EQ, broadband compression, stereo width/mono-bass, then true-peak limiter.

## Default target

The `streaming-normal` configuration uses -14 LUFS integrated and -1 dBTP. These defaults follow Spotify's published normal-normalization and mastering guidance. They are configuration values rather than universal claims. Broadcast workflows can instead supply the EBU R128 target of -23 LUFS and -1 dBTP without changing engine code.

- Spotify loudness normalization: https://support.spotify.com/artists/article/loudness-normalization/
- EBU R 128: https://tech.ebu.ch/files/live/sites/tech/files/shared/r/r128-2014.pdf

## Rules

- Spectral peaks at or above the configured local-excess threshold produce bounded corrective cuts only. No boost is proposed.
- Broadband compression is bypassed unless measured crest factor crosses its configured threshold. Makeup gain is always initially zero.
- Width is preserved at 1.0 unless correlation or measured mono-sum loss crosses a configured safety boundary. The v1 engine never proposes widening above 1.0.
- Limiter input gain is the minimum of target-loudness gain, measured true-peak headroom, and configured maximum input gain. The true-peak ceiling therefore wins over the loudness target.

Thresholds that are not external delivery specifications are exposed as named configuration policy, logged in each rule expression, and tested. They are not presented as measured facts or confidence values.
