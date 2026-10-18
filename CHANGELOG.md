# Changelog

Version history and technical notes for **SikkenWorks B1**, the drawbar-organ firmware for the M-VAVE FM-1.

This file contains release-specific changes, build validation, performance measurements and outstanding hardware-verification limits. For an overview of the instrument and its controls, see the [README](README.md).

---

## V2.6 — Realtime polyphony & audio stability

### Changed

- **Dense-polyphony fast mixer:** when at least three sustained voices are stable, shared tonewheel harmonic coefficients are combined once per PCM chunk. Each shared wheel is rendered once per sample, regardless of how many held notes use it. All 12 voices, nine drawbars and MIDI/range routing remain available.
- **Accurate note transitions:** note-on, note-off, drawbar changes and key-click onset automatically use the original sample-by-sample, per-voice path.
- **Reduced idle work:** the firmware skips scans of all 27 pending chord-key queues when there is no queued CHORD ATTACK note. Performance routing only updates when the physical-key bitset changes.
- **Output gain path:** two variable 64-bit per-sample divisions have been replaced by Q15 fixed-point shifts. Conservative reciprocal tables replace further runtime variable divisions in the input/output peak controls. Limiting and headroom protection remain active.
- **Drawbar smoothing:** moved to an economical 16-sample control interval while retaining smooth audible transitions.
- **Factory percussion:** enabled by default, using the third harmonic on newly created or reset projects. Existing saved-project percussion values are preserved.
- No planned feature removals: UPPER/LOWER/SOLO, key click, leakage, percussion, Leslie controls, scanner C/V, B3/ROCK amps, delay, four reverb types, chords, humanized attack, 48 scales, 12 persistent projects, OTA/stock/BEETLE options, startup branding and the native UI remain.

### Validation

- Two repeated builds from the final pi32v2 firmware source produced byte-identical firmware and `.fwsc` packages.
- OTA CRC, BEETLE protected header bytes and embedded WebInstaller image hash were checked against the packaged update.
- Historical DSP, MIDI, chord, keyboard-range and project regression tests passed.
- 16 demanding chord cases and eight longer reference-versus-optimized mixer cases covered B3/ROCK, full/SAFE DSP and effects on/off. Across the tested sustained twelve-note chords, mean audio-output deviation remained below 0.5% relative to the reference mixer; integer output is not bit-identical.
- AddressSanitizer/UndefinedBehaviorSanitizer host stress tests passed. Note-off eventually returned to zero PCM output.
- All 62 code-derived, 240 × 240 UI captures were regenerated and checked.
- In the existing `native_audio_perf.c` desktop-host benchmark with 12 held notes and scanner/rotor, the optimized path measured approximately **39–41% less per-sample computation** than V2.5 (seven-run median on the same host). This is **not** a 60 MHz FM-1 hardware-timing measurement.
- A separate all-effects comparison reported latency similar to V2.4. Benchmark configurations and version baselines differ, so these results should not be treated as the same measurement.
- New three-second B3 and ROCK reference WAVs are rendered by the C DSP in software, **not recorded from a physical FM-1**.

### Known limits

- V2.6 has **not been verified running on a physical FM-1**. Offline tests do not establish that underruns, audible clicks or distortion are eliminated, nor that flash, boot, project migration or rollback will succeed on every device.
- Preserve an independent working recovery path and the original BEETLE package. Do not erase unknown flash regions to resolve storage issues.

## V2.5 — Cleaner signal path

### Changed

- Added a polyphonic peak follower that reduces excessive chord gain **before** the amp, scanner and cabinet stages. Attack is immediate; recovery is slower.
- Removed mandatory soft clipping before the scanner in the B3/LESLIE SAFE-DSP route and removed always-on final stereo soft clipping from both routes, preserving more of the summed tonewheel signal.
- Added a linked-stereo final peak limiter to keep output transients under the PCM ceiling without disrupting the left/right balance.
- Retained an additional approximately **−2.5 dB output trim** to avoid increasing FM-1 output level unexpectedly.
- ROCK/LORD retains user-controlled amplifier saturation; high AMP GAIN / DRIVE settings are intentionally distorted without multiple unintended clipping stages.
- No additional heap allocation, flash write, UI control, storage-format change, user-project migration or OTA/recovery entry change.

### Validation and limits

- Sixteen native stress cases tested one to twelve voices, nine fully open drawbars, B3/ROCK, regular/SAFE DSP and effects on/off. Tested PCM peaks stayed below **24,500 / 32,767**, and UBSan checks passed.
- A user-recorded video did not indicate full-scale digital clipping **after the phone microphone**. That does not rule out earlier signal-path distortion or audio DMA underruns.

## V2.4 — Factory rotor defaults

### Changed

- New/reset projects use **FAST** rotor mode by default.
- New/reset projects use **ROTOR MIX 60** instead of 90.
- Initial rotor phase increments now match FAST to avoid a hidden SLOW-to-FAST warm-up.
- Existing projects continue to load their saved rotor mode and mix without migration or rewriting.
- Organ parameters, effects, project format, boot animation and other firmware paths remain present.
- Made 62 pixel-exact **240 × 240** firmware screen images available as a separate ZIP of individually named PNGs.

## V2.3 — Effects and project handling

### Changed

- **Scanner chorus/vibrato:** replaced stepped integer tap positions with fractional-delay, 8-bit-interpolated scanning in full and SAFE-DSP paths. Added smoothed dry/wet transitions to address a known DSP-level source of zipper noise; the physical-device result still requires listening tests.
- **FX 5/6, DELAY:** K1 MIX (0–100); K2 TIME (0–100, approximately 32–350 ms); K3 FEEDBACK (0–90); K4 STEREO (0–100).
- **FX 6/6, REVERB:** K1 TYPE (OFF, SPRING, ROOM, HALL, CATHEDRAL); K2 MIX (0–100); K3 DECAY (0–100); K4 DAMP (0–100). The reverb algorithms use original low-memory integer DSP, not convolution impulse-response samples.
- **Effect tails:** bounded delay and reverb tails continue after the last note; dry registrations still return to silence. No unkeyed tonewheel signal is mixed into the tails.
- **Project loading:** choose one of 12 slots using K1 or the SELECT encoder and confirm with the physical SEL button (upper row, second from left). ALGORITHM is not used for confirming a load.
- **Project feedback:** `PROJECT SAVED` and `PROJECT LOADED` are shown for approximately 1.6 seconds; errors have separate messages.
- **Complete project storage:** drawbars for all three organ registrations; four keyboard ranges and chord ATTACK; chord assignments; scale/root/KEYLOCK/GUIDE; percussion, key click, leakage, Leslie controls, B3/ROCK amp, chorus/vibrato, rotor speeds, delay/reverb, global octave and organ-bank selection.
- **Compatibility:** reads the schema 1 / project V5 format from B1 V2.2 and migrates it to schema 2 / project V6. The next successful save commits the new copy-on-write journal without overwriting the previous flash sector until the new archive is verified.
- Retained the SikkenWorks particle startup, safe mode, BEETLE-compatible OTA header and updater, and the supplied BEETLE backup.

---

## Firmware packaging and verification notes

For the V2.6 package:

- `build/SikkenWorks-B1-V2.6-BEETLE-OTA.fwsc` is a JieLi application-update candidate checked against the BEETLE RC1.01 header reference. Physical installation is **unverified**.
- `B1-WebInstaller.html` embeds the same update image and requires a compatible WebMIDI browser and device.
- `build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc` is the unchanged original BEETLE reference; it is not a promise of a universally safe rollback path.
- Full source, build/test tools, regenerated UI assets and reference audio are included in their respective directories.

Neither a successful offline `.fwsc` check nor a software reference recording proves boot success, DAC audio integrity, real-time scheduling margin or hardware recovery behavior.


# B1 V2.7

- FIX: CHORD ASSIGN no longer requires an invisible manually configured range on a new project.
- NEW: K3 ASSIGN ON/OFF with safe range enabling. Defaults OFF, preserving all organ keys.
- NEW: K4 CHORD ATTACK 0–100 directly on CHORD ASSIGN, alongside existing RANGE editor control.
- FIX: K2 CHORD only chooses the pending quality; a physical keyboard press makes the assignment.
- FIX: OFF mode no longer produces spurious outside-range errors.
- SAVE: chord switch persisted in existing V6 project padding without changing structure length; backwards compatible.
- UI: two additional exact screenshots; all 64 frames rebuilt. No other sound/FX changes.
- PACKAGE: ID FM-1_2127 and SHA-verified WebInstaller with prior B1 and BEETLE support.
