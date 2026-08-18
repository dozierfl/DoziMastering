# Dozi processor hardware view

The Chain view supports two presentations of the same processor state:

- **Hardware** (default): a tactile, processor-specific faceplate with knobs, switches, meters, and activity displays.
- **Technical**: the existing precise inspector with named parameters and numeric editing.

Switching presentation must not change the plan, bypass state, playback, selection, or undo history. Both views bind to the same parameter model. A value changed in either view updates the other immediately.

## Shared interaction rules

- Clicking a chain card selects one processor and opens its faceplate.
- The illuminated **IN** control toggles bypass and participates in command history.
- Knobs support vertical drag, fine adjustment with Shift, double-click numeric entry, and reset to the style value.
- Every knob always has a nearby numeric readout; visual position is never the only indication.
- Meters and graphs are read-only visualizations of measured or live processor state.
- **AUTO** applies an analysis-backed recommendation only after displaying the pending change.
- **RESET** restores the selected style's values, not arbitrary hard-coded defaults.
- Unsupported controls shown in a concept image must not appear in production until the DSP implements them.
- Hardware controls require keyboard focus, accessible names, value announcements, and a non-color state indicator.

## Processor presentations

| Processor | Primary hardware controls | Primary display |
|---|---|---|
| Corrective EQ | Low, Low Mid, High Mid, Air, In, Auto, Reset | EQ curve and corrective nodes |
| Dynamic EQ | Per-band frequency, threshold, range, listen | Spectrum and dynamic reduction |
| De-Esser | Frequency, threshold, range, release, listen | Sibilance detection and reduction |
| Glue Compressor | Threshold, attack, release, ratio, makeup, sidechain HPF | Input/output and gain reduction |
| Multiband Compressor | Per-band threshold/range, crossovers, solo | Band envelopes and per-band reduction |
| Saturation | Drive, character, mix, output, mode, HQ | Transfer curve and harmonics |
| Width / Mono Bass | Width, mono-below, side trim, balance, mono check | Vectorscope and phase correlation |
| True-Peak Limiter | Input, ceiling, release, lookahead, link, true peak | L/R output, gain reduction, peak events |

The PNGs in `design/processor-hardware-mockups/` are visual direction only. Production controls should be drawn and animated in JUCE so they remain crisp, scalable, interactive, and accessible.
