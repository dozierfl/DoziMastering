# Phase 2 loudness targets and mastering versions

Verified 2026-08-08 from Spotify's first-party artist documentation:

- Spotify Normal: -14 LUFS integrated, -1 dBTP ceiling.
- Spotify Loud: -11 LUFS integrated. Dozi uses -2 dBTP because Spotify explicitly
  recommends true peak below -2 dB for masters louder than -14 LUFS.
- Spotify Quiet: -19 LUFS integrated, with the published -1 dBTP mastering ceiling.

Source: <https://support.spotify.com/us/artists/article/loudness-normalization/>

Dozi does not label Apple Music or YouTube values as official presets because a
current first-party mastering target was not verified. Users may still enter a
custom target; the UI must describe it as custom rather than platform-certified.

Mastering versions use the `DOZI_MASTERING_VERSION 1` format and contain plan
schema 2. Saving writes a `.partial` file and commits it atomically. Restoring
rejects unsupported format or plan schema versions. A version contains the
target, module order and types, bypass states, parameters, and complete decision
traces. Version comparison reports target and module-setting differences without
rendering or modifying audio.
