# Dozi Mastering Presentation Refactor Plan

## Objective

Refactor Dozi Mastering from a single dense engineering surface into a focused, premium mastering console. The presentation should take structural inspiration from Lurssen Mastering Console—clear view separation, persistent metering and transport, prominent styles, and progressive disclosure—while retaining an original Dozi visual identity and Dozi's deeper analysis, target, conversational, versioning, and delivery features.

This is a presentation and interaction refactor. Existing DSP, analysis, rendering, project, recovery, recent-session, transport, and export behavior should remain functionally stable unless a phase explicitly calls for a related improvement.

## Research findings

IK Multimedia presents Lurssen Mastering Console as three primary views:

1. **Studio** — the approachable main mastering surface with Style selection, Input Drive, five fixed EQ bands, Push, Input/Process VU meters, bypass, and stereo/mono monitoring.
2. **Wave** — a large waveform used for navigation, loops, automation display, and automation editing.
3. **Chain** — the detailed signal path, exposing the processors and a limited set of deeper controls.

The official product material emphasizes 40 Styles, a small number of macro controls, assignable Input/Process metering, stereo/mono monitoring, automation, project portability, and multi-format export. The UI screenshots consistently preserve a console/meters/transport foundation while changing the task-specific upper panel.

Sources:

- [IK Multimedia product overview](https://www.ikmultimedia.com/products/lurssen/index.php?p=info)
- [IK Multimedia specifications](https://www.ikmultimedia.com/products/lurssen/index.php?p=specs)
- [Official Studio view image](https://www.ikmultimedia.com/products/lurssen/images/1.0/GUI-REF/Lurssen_iPad_Main%402x.jpg)
- [Official Wave view image](https://cn.ikmultimedia.com/products/lurssen/images/1.0/GUI-REF/Lurssen_iPad_Wave_lgr%402x.jpg)
- [Chain view reference](https://medias.audiofanzine.com/images/normal/1406556.png)
- [IK project-transfer workflow](https://www.ikmultimedia.com/faq/?id=1285)

## Product direction

Dozi should adopt the clarity of a mastering console, not copy Lurssen's branding or exact hardware artwork.

### Dozi design principles

- **Music first:** the loaded song, current style, target, meters, and transport are always obvious.
- **Progressive disclosure:** everyday mastering controls are visible; engineering details are one click away.
- **One task per view:** audition on Console, navigate on Wave, edit on Chain, validate and export on Deliver.
- **Persistent listening context:** switching views must never stop playback, move the playhead, alter the loop, or change A/B state.
- **Calm visual hierarchy:** fewer borders, fewer equal-weight controls, larger primary controls, and more negative space.
- **Original Dozi identity:** dark graphite and deep navy surfaces, restrained cyan/teal signal color, warm amber warnings, and off-white typography. Avoid copying Lurssen's brick wall, equipment faces, labels, knob artwork, or trade dress.
- **Engineering confidence:** critical values remain numeric and inspectable even when represented by knobs or meters.
- **Conversation is a first-class workflow:** Talk to Dozi should feel like an assistant strip, not another technical parameter row.

## Proposed application architecture

### Persistent top bar

The top bar remains visible in every view:

- Dozi Mastering logo/name
- Current project/song name
- Source format summary: sample rate, bit depth, stereo/mono, duration
- Style selector
- Delivery target selector
- Undo/redo
- Save status
- Project menu: New, Open, Save, Save As, Recent Sessions
- View switcher: **Console · Wave · Chain · Deliver**

The current workflow sentence (`LOAD > ANALYZE > AUDITION > EXPORT`) should be removed from the permanent interface. Progress can instead appear as compact state badges beside the song name: Loaded, Analyzed, Audition Ready, Exported.

### Persistent bottom transport

The transport remains fixed across every view:

- Return to start
- Play/pause
- Loop track
- Loop selection
- Time elapsed / total duration
- Compact overview scrubber
- Original/Mastered A/B
- Loudness Match
- Stereo/Mono monitoring
- Monitor level and Mute
- Audio device menu

Space and Return retain their existing global behavior outside text-entry fields.

### Persistent meter bridge

A compact meter bridge remains visible above the transport:

- Stereo peak bars
- RMS bars
- Integrated LUFS
- Short-term LUFS
- True peak
- Gain reduction
- Input/Processed meter selector

Console view may expand this bridge into larger meters; the other views use the compact version.

## View 1: Console

This becomes the default working view and the visual centerpiece.

### Layout

```text
┌──────────────────────────────────────────────────────────────────────┐
│ Project / Song       Style       Target       CONSOLE WAVE CHAIN ...│
├──────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  INPUT            TONAL SHAPE             MASTER / OUTPUT            │
│  large knob     Low  Body  Presence  Air    large knob               │
│  Input Gain                                  Output Ceiling           │
│                                                                      │
│       ┌──────── Input / Process meter bridge ────────────────┐       │
│       │ L/R Peak · RMS · LUFS · True Peak · Gain Reduction  │       │
│       └──────────────────────────────────────────────────────┘       │
│                                                                      │
│  [Original / Mastered] [Loudness Match] [Mono] [Assistant panel]    │
├──────────────────────────────────────────────────────────────────────┤
│ Transport · Loop · Time · Monitor Level · Device                     │
└──────────────────────────────────────────────────────────────────────┘
```

### Primary controls

- **Input Gain** — visible, with a clear numeric dB readout.
- **Target Loudness** — visible as a target value, but visually secondary to listening controls.
- **Output Ceiling** — visible with true-peak units.
- **Tonal Shape macros** — four approachable controls mapped to safe internal processing:
  - Low: broad low-frequency balance
  - Body: low-mid density
  - Presence: upper-mid clarity
  - Air: high-frequency openness
- **Width** — compact stereo-width control with mono-compatibility warning state.
- **Character** — macro for saturation amount when the selected Style uses saturation.

Dozi should not expose every compressor, de-esser, crossover, saturation, and limiter parameter on this page.

### Talk to Dozi

Use a collapsible assistant panel on the right or a drawer from the bottom:

- Conversation history
- Text entry
- Future microphone button
- Suggested phrases
- A visible summary of the pending change, for example: `Low: -0.50 dB @ 80 Hz`
- Apply, Undo, Redo, and Keep controls
- Audition status and progress

The assistant should describe what it changed in plain language while offering a disclosure link to the exact processor settings.

## View 2: Wave

Wave view is for navigation, comparison, loops, and future automation.

### Layout and behavior

- Large true stereo waveform with distinct left and right lanes
- Time ruler in minutes and seconds
- Playhead aligned exactly with the persistent scrubber
- Drag selection with handles
- Loop-region shading
- Markers for intro, verse, chorus, bridge, and outro when analysis supports them
- Original/Mastered waveform overlay or toggle
- Optional loudness trace and true-peak event lane
- Conversational revision markers, such as `Low -0.5 dB`, at the time or region affected
- Zoom controls and fit-song button

Future automation belongs here, but the first refactor should not imply automation capabilities that are not implemented.

## View 3: Chain

Chain view replaces the current grid of module toggles plus the permanently visible generic inspector.

### Signal-flow strip

Display the actual Dozi chain horizontally:

`Corrective EQ → Dynamic EQ → De-Esser → Glue → Multiband → Saturation → Width/Mono Bass → True-Peak Limiter`

Each module card shows:

- Enabled/bypassed state
- One primary value
- Small activity or gain-reduction meter
- Health color: neutral, active, caution
- Short reason supplied by the analysis/decision trace

Clicking a card opens a focused inspector below or at the right. Only the selected processor's controls appear. Advanced parameters live under an **Advanced** disclosure.

### Chain interaction rules

- Dragging modules should not be offered unless the DSP engine truly supports reordering.
- Bypass must be click-safe and clearly distinguish bypassed from merely inactive.
- Parameter names should be humanized; raw map keys must never appear in the production UI.
- Every user change should enter the same undo history as conversational changes.
- A/B comparison can compare the whole chain or the currently selected module.

## View 4: Deliver

Dozi needs a dedicated finalization view because its analysis and export capabilities exceed Lurssen's simplified export workflow.

### Validation summary

- Before/after LUFS
- RMS
- Sample peak and true peak
- Crest factor
- Clipped samples
- Phase correlation and mono compatibility
- Limiter maximum gain reduction
- Target compliance badge
- Warnings with recommended action

### Export panel

- Format: WAV, MP3, and later AIFF/FLAC/AAC as implemented
- Bit depth/sample rate where supported
- MP3 bitrate
- Dither where applicable
- Naming template and destination
- Replace Current Export / Create New File
- Export versions list
- Reports: TXT/JSON, with a future human-readable PDF option
- Large single **Export Master** action

Technical details should be available without dominating the main Console view.

## Empty and loading states

### No session

Use a focused start screen rather than an empty console:

- Drop a mix
- Choose Mix
- Open Project
- Recent Sessions
- Supported-format note

### Loaded but not analyzed

- Show waveform and source meters.
- Disable mastering controls with one clear explanation.
- Make **Analyze Mix** the primary action.

### Rendering

- Preserve the current view and playback position.
- Show unobtrusive progress in the assistant/status area.
- Keep Cancel available.
- Avoid disabling unrelated navigation and project controls.

## Current-to-new control mapping

| Current element | New location |
|---|---|
| Style preset | Persistent top bar |
| Spotify/custom target | Persistent top bar; detailed value on Console |
| Target LUFS and ceiling sliders | Console |
| Waveform and selection | Wave view; compact scrubber persists |
| Choose/reference/analyze/render | Project menu and contextual primary action |
| Export | Deliver view |
| Original/Mastered and Loudness Match | Persistent transport/meter bridge |
| Loop controls | Persistent transport and Wave view |
| Monitor level, mute, audio device | Persistent transport |
| Save/open/recent projects | Project menu |
| Save/restore/compare versions | Version menu and Deliver/version drawer |
| Module toggles | Chain module cards |
| Generic module inspector | Chain selected-module inspector |
| Before/after summaries | Deliver validation summary |
| Technical results | Deliver technical disclosure |
| Talk to Dozi text row | Collapsible assistant panel |

## Visual system

### Color tokens

- Canvas: `#080B10`
- Surface 1: `#111722`
- Surface 2: `#182230`
- Raised control: `#222D3A`
- Primary signal: `#38D8BE`
- Secondary signal: `#4FA8D8`
- Warm active/accent: `#E6A85C`
- Warning: `#F0B34F`
- Error/clip: `#F05D64`
- Primary text: `#F2F5F7`
- Secondary text: `#9BA8B5`

The values are starting tokens and require contrast testing in implementation.

### Typography

- Use one modern macOS-appropriate sans family.
- 11–12 pt for secondary values and menus.
- 13–14 pt for controls.
- 16–18 pt for panel titles.
- 24–30 pt only for song identity or major meter values.
- Numeric readouts use tabular figures.

### Controls

- Knobs are reserved for continuous listening-oriented macros.
- Sliders are used for ranges where relative position matters.
- Numeric entry remains available via click/double-click.
- Toggle buttons use consistent illuminated states.
- Tooltips explain both the sonic result and whether the parameter affects export or monitoring only.

## Refactor architecture

Before rebuilding the visual hierarchy, separate application state from individual JUCE controls.

### Proposed components

- `MasteringSessionModel` — source, reference, analysis, plan, versions, recovery, dirty state
- `MasteringPlaybackController` — transport, loops, A/B, loudness matching, monitor gain, device
- `MasteringRenderController` — preview/export jobs and cancellation
- `MasteringCommandHistory` — manual and conversational undo/redo
- `MasteringShell` — header, view router, compact meter bridge, transport
- `ConsoleView`
- `WaveView`
- `ChainView`
- `DeliverView`
- `DoziAssistantPanel`
- `DoziLookAndFeel`

This prevents another monolithic `MasteringComponent` and allows presentation changes without risking DSP state.

## Delivery phases

### Phase 0 — State and regression boundary

- Capture screenshots and behavioral checks for every current feature.
- Add view-model/controller seams without visual change.
- Consolidate project dirty-state and command history.
- Establish keyboard, transport, and render regression tests.

Exit criterion: all existing tests pass and the current UI behaves identically.

### Phase 1 — Design system and persistent shell

- Implement `DoziLookAndFeel` tokens, typography, spacing, focus, and control states.
- Build persistent top bar, view switcher, meter bridge, and transport.
- Add scalable layout constraints and a practical minimum window size.

Exit criterion: shell works at supported sizes with no clipped controls.

### Phase 2 — Console view

- Build the new primary console and macro controls.
- Move style/target selection into the header.
- Add expandable Talk to Dozi panel.
- Preserve exact numeric editing and A/B behavior.

Exit criterion: a user can load, analyze, choose a style/target, audition, converse, and compare without opening an advanced view.

### Phase 3 — Wave view

- Build stereo waveform lanes, ruler, zoom, selection handles, loops, and overlays.
- Reuse the corrected shared playhead geometry.
- Add revision markers; defer unsupported automation editing.

Exit criterion: navigation and looping remain sample-consistent across views.

### Phase 4 — Chain view

- Build real signal-flow cards and module activity meters.
- Replace raw generic parameters with typed, human-readable controls.
- Integrate unified undo/redo.

Exit criterion: every currently editable module parameter remains accessible and project-compatible.

### Phase 5 — Deliver view

- Consolidate validation, warnings, versions, formats, naming, replace/create behavior, and reports.
- Clearly separate monitoring level from rendered output level.

Exit criterion: every supported output can be configured and verified from one view.

### Phase 6 — Polish and validation

- Accessibility labels and keyboard navigation
- Retina rendering and resize testing
- Color-blind and contrast review
- VoiceOver pass
- M1 performance profiling
- Long-file and rapid-view-switch stress tests
- Visual snapshot tests for each state and view

## Acceptance criteria

- A first-time user can identify Load, Analyze, Style, Target, Play, A/B, and Export within five seconds.
- No view exposes unrelated advanced controls by default.
- Playback continues seamlessly when switching views.
- Style or target changes visibly identify pending versus rendered state.
- Monitoring volume can never be confused with mastering gain.
- The original source can never be overwritten by export.
- The selected project, view, playhead, loop, and assistant history recover on relaunch where supported.
- All current factory presets, Suno WAV Remaster behavior, project files, recent sessions, MP3 bitrates, and replace/create export behavior remain available.
- The interface remains usable at the minimum supported window size and scales cleanly on Retina displays.

## Recommended first design deliverable

Create high-fidelity static mockups for these six states before implementation:

1. Empty/start screen
2. Console after analysis
3. Console with Talk to Dozi open
4. Wave with a loop selection
5. Chain with True-Peak Limiter selected
6. Deliver with a verified WAV and MP3 configuration

Approve the visual hierarchy and interaction placement from those mockups before changing production JUCE layout code.
