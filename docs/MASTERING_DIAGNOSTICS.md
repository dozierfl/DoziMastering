# Perceptual mastering diagnostics

Dozi translates measured audio characteristics into testable perceptual hypotheses. A diagnosis is not treated as proof that a listener will hear a problem and never applies processing automatically.

Each finding contains:

- a mastering descriptor such as muddy, harsh, squashed, peaky, or phasey;
- the diagnostic family;
- the measurement or level-matched reference difference that triggered it;
- deterministic evidence strength, not statistical confidence;
- a bounded recommendation;
- whether dynamic processing or a return to the mix is preferred.

## Supported evidence

Objective findings currently use localized resonances, clipped-sample count, crest factor, stereo phase correlation, and mono compatibility. Tonal-balance findings compare the source with a user-selected reference. The comparison removes the median full-spectrum level difference before testing individual bands, preventing overall loudness from being mislabeled as a tonal imbalance.

The current spectral bands are sub, low, low-mid, mid, high-mid, high, and air. A localized resonance within an elevated band makes the recommendation favor dynamic processing.

## Genre profiles

Dozi does not infer genre from filenames and does not claim that one generic curve is normal for a genre. A production genre profile must be derived from a licensed or user-provided, labeled corpus and should include distributions by band, loudness, dynamics, and stereo behavior rather than a single average curve. Profiles must be versioned with their sample count, selection criteria, percentile ranges, and analysis algorithm version.

Until those calibrated profiles exist, users can select a trusted reference track for style-relative tonal diagnosis. The API reserves a `genreProfile` evidence basis so calibrated profiles can be added without changing the finding model.
