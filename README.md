# SikkenWorks B1 V2.6 — Realtime Polyphony & Audio Stability

Native firmware for M-VAVE FM1. Complete source included under GPL-3.0-only.

## V2.6 native realtime improvements

- **Dense polyphony fast mixer:** when three or more sustained voices are stable, combine the harmonic coefficients of shared tonewheels once per PCM chunk. Render a wheel once per sample regardless of how many held notes use it. All 12 voices, every drawbar and MIDI/range routing remain present. At note-on, note-off, changing drawbars or key-click onset, the original sample-by-sample per-voice path is automatically used.
- **No idle chord scheduling scans:** pending CHORD ATTACK sample-timing is kept, but B1 no longer scans all 27 key queues for every audio block if no chord note is waiting. A note press/release only updates performance routing when the actual physical key bitset has changed.
- **Realtime gain path:** avoid two 64-bit variable divides every sample in the stereo output guard; fixed-point Q15 shifts instead. Conservative reciprocal lookups replace runtime variable divisions in the input/output peak controls. The limiter and headroom protection stay active.
- **Drawbar smoothing:** calculate the same gradual motion at a more economical control rate (16 samples). Actual values still glide rather than abruptly jumping.
- **Percussion default ON**, 3rd harmonic on new/reset projects; saved projects keep their own percussion setting, as well as all other parameters.
- **No features removed:** three manual registrations, key click, leakage, percussion, Leslie slow/fast/fine speed, scanner C/V, ROCK/B3 amps, delay, four reverbs, chords and humanized attack, 48 scales, 12 named persistent projects, OTA/stock/BEETLE menu, startup branding and the FM1 UI remain.

### Tests and limits

- Real pi32v2 firmware build, two identical repeated builds of **the final source**; OTA .fwsc CRC and byte-for-byte BEETLE protected header audit; embedded WebInstaller hash matches the .fwsc exactly.
- Complete historical native tests, 16 heavy chord cases, 8 long-window reference-vs-fast mixer cases (B3/ROCK, full/SAFE DSP, FX on/off). Mean audio output deviation remains below 0.5% in tested sustained twelve-note chords (integer rounding, not bit-identical).
- AddressSanitizer and UndefinedBehaviorSanitizer host stress tests passed; note-off ultimately produces zero PCM output; all 62 code-derived 240 x 240 screenshots regenerated and checked.
- On the **same desktop host** the existing native_audio_perf.c benchmark with 12 held notes and scanner/rotor shows ~39–41% less per-sample compute time than V2.5 (seven-run median on the same host). This is *not* a 60MHz/FM1 timing measurement and does not guarantee no hardware underruns.
- The current physical FM1 has not yet been measured running V2.6. Do not claim a guaranteed cure for clicking, distortion or a safe rollback. Preserve an independent working recovery path and the original BEETLE package.

## Inherited V2.5 clean signal path

- A polyphonic peak follower now applies protective gain **before** any
  amplifier/scanner/cabinet stage.  This avoids feeding excessive full-drawbar
  chord sums into non-linear stages. Attack is immediate; gain recovers slowly.
- The B3/LESLIE SAFE-DSP route no longer performs mandatory soft clipping
  before the scanner, nor does either route apply an always-on soft-clip to
  the final stereo output. This preserves the actual summed tonewheel wave.
- A linked-stereo final peak limiter keeps transients below the PCM ceiling
  while preserving L/R image.  The output keeps an additional -2.5 dB trim
  so the cleaner pipeline does not suddenly become hotter on the FM1.
- ROCK/LORD retains its **user-controlled** amplifier saturation. It remains
  deliberately distorted at high AMP GAIN / DRIVE, but dense registrations
  are no longer forced through multiple unintended blanket saturators.
- No new heap allocations, flash writes, UI controls, storage format changes,
  user project migrations, or changes to OTA and recovery entry.
- The recorded user video does not show digital full-scale clipping *after*
  the phone microphone; upstream distortion and DMA underruns remain possible.

## Validation

- 16 native stress tests cover 1/12 voices, all nine drawbars fully open,
  B3/ROCK, normal/SAFE DSP, plus effects OFF/ON. They complete with PCM peaks
  strictly under 24,500 of 32,767 and UBSan clean.
- Pre-existing native DSP/MIDI/chord/range/12-project regressions pass.
- Two repeated final V2.6 pi32v2 builds and `.fwsc` packages are byte-identical.
- 62 code-exact UI PNGs regenerated from C renderer; no graphics invented.
- New 3-second B3 and ROCK WAVs are **software** audio references, not
  recordings from the physical FM1.
- CPU benchmark on desktop host with 12 voices/all effects shows similar
  latency to V2.4. FM1 realtime timing, DAC output and recovery need a
  physical hardware test. Do NOT claim a guaranteed audible fix yet.

## V2.4 inherited factory defaults (only new/reset projects)

- Factory rotor is **FAST**.
- Factory ROTOR MIX is **60** (previously 90).
- Rotor initial phase increments agree with FAST so there is no hidden SLOW-to-FAST warm-up on a new/reset project.
- Existing saved projects keep their saved rotor mode and mix when loaded; no automatic project migration or rewriting.
- The organ, effects, project format, boot animation and other code paths are retained.
- 62 pixel-exact 240 × 240 native firmware screens can be downloaded as one separate ZIP containing individually named PNGs.

## Changes inherited from V2.3

- **Chorus/vibrato:** a fractional-delay 8-bit interpolated scanner replaces stepping between integer tap locations in both the full and SAFE-DSP paths. A smoothed dry/wet ramp prevents abrupt mode changes. This removes the known zipper-noise source at the DSP-code level; the physical FM1 still needs a listening test.
- **FX 5/6 — DELAY:** K1 MIX (0–100), K2 TIME (0–100; ~32–350 ms), K3 FEEDBACK (0–90), K4 STEREO (0–100).
- **FX 6/6 — REVERB:** K1 TYPE (OFF, SPRING, ROOM, HALL, CATHEDRAL), K2 MIX (0–100), K3 DECAY (0–100), K4 DAMP (0–100). These are original low-memory integer DSP algorithms, not a sampled impulse-response library.
- **Effect tails:** intentional delay/reverb tails fade for a bounded duration after the last key release. No unkeyed tonewheel signal is mixed into them. Earlier complete-silence behavior remains for dry presets.
- **Project loading:** choose one of 12 slots with K1 or turning SELECT, then press the physical **SEL** button (upper button row, second from left). ALGORITHM is not needed.
- **Visible saved/loaded feedback:** `PROJECT SAVED` or `PROJECT LOADED` appears for 1.6 s. Errors remain separately shown.
- **All musical parameters saved:** drawbars for UPPER/LOWER/SOLO; all four ranges including chord ATTACK; chord assignments; scale/root/KEYLOCK/GUIDE; percussion, click, leakage, Leslie settings, B3/ROCK amp, chorus/vibrato, rotor speeds; delay and reverb; global octave and selected organ bank.
- **Old project compatibility:** flash journal schema 1 / project V5 (from B1 V2.2) is read and migrated to schema 2 / project V6; the next successful save writes the new schema as a copy-on-write journal. The old sector is not overwritten before the new committed archive is verified.
- **Startup and recovery:** SikkenWorks particle startup, safe mode, BEETLE-compatible OTA header and updater are retained. BEETLE backup included.

## Firmware package

- `build/SikkenWorks-B1-V2.6-BEETLE-OTA.fwsc` — real JieLi application update candidate; not yet physically verified.
- `B1-WebInstaller.html` — standalone web installer embedding the exact same `.fwsc`. Requires a supported browser/WebMIDI and device compatibility.
- `build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc` — unmodified original BEETLE reference.
- `firmware/` — complete C firmware sources, JieLi build and Felucca HAL.
- `tools/`, `tests/` — build, verification, audio and pixel-exact UI test tooling.
- `ui-captures/` — 62 RGB565 firmware snapshots and matching PNGs, GIF and manifest.
- `audio-reference/` — 7 three-second WAV renders made by the exact native C DSP.

## Installation caution

The resulting `.fwsc` is packaged and offline verified against a BEETLE RC1.01 header reference. This **does not prove** flashing, booting, audio timing, saved-project migration, or rollback on the physical FM1. Keep a proven independent recovery method. Back up projects before any experiment. Never erase unknown flash regions merely to solve a storage error. These limits matter especially when the installed firmware or hardware revision differs from the supplied BEETLE reference.

## Copyright and credits

SikkenWorks B1 — GPL-3.0-only. Felucca's platform hardware abstraction, USB/OTA layer and other relevant files remain copyright © Leo Kuroshita / Hügelton Instruments and their contributors under GPL-3.0-only. See `CREDITS.md` and the supplied license texts. `INTER-OFL-1.1.txt` also remains included as applicable to the font.
