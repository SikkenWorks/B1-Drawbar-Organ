## V4.3 Installer HF1 (firmware unchanged)

- Prevent Advanced Mode from unchecking itself when choosing another firmware.
- Add an explicit, opt-in source compatibility path for shared FM-1_900 identities with read-only identity identification and protocol-checked flashing stages.
- Keep existing pinned firmware hashes, known protected boot header and loader checks; require the expected loader identity.
- See `INSTALLER-HF1-VERIFICATION.md` for limitations and hardware-test status.

## V4.3 — October 19, 2026 (10 kHz keyboard IRQ: lossless settled-key fast path)

- **No concessions to sound or responsiveness.** Retain V4.2's exact `b1_engine.c`, `b1_performance.c`, 91 virtual tonewheels, 12 voices, all nine drawbars, ROCK/B3, ORGAN/SAW, Leslie, effects, MIDI, saved projects, KEY ASSIGN persistent LEDs, 10kHz physical GPIO/LED scan and 2kHz USB polling.
- Optimize only the FM1 input matrix's *software debounce*. Every column is still sampled and its LEDs latched at the exact same 10kHz timer rate. For a stable column, where no relevant key-row bits have changed and all debounce counters are zero, bypass the redundant per-key C handler calls. **A physical key change or pending debouncer immediately uses the unmodified press=2-frame / release=8-frame algorithm.** Unrelated encoder row changes do not wake the key handler. Note-on and note-off scheduling remains on the original scan tick.
- Replace the runtime column selection shifts/XORs in the high-frequency ISR with 11 **precomputed byte-identical 74HC595 shift-register values**. No changes to bit-banging, LED dim timing, encoder scan or GPIO waveforms are intended.
- Add deterministic 1,100,000-column reference-versus-optimized simulation over overlapping key presses, contact bounce, long idle, button edges and encoder-only input bits; exact note state, button state, edge timing, debounce counters and stored note timing match. 4,100,000 reference per-key calls vs 108,153 optimized in this scenario, **97.4% fewer calls (NOT 97.4% CPU reduction)**. Eleven column words and key masks independently checked against the original equations and matrix map.
- Retain V4.2 pi32v2 foreground IRQ-idle instructions and V4.1 DSP Smart Idle; unchanged `b1_engine.c` still produces exact multi-stage PCM plus full internal DSP state vs reference V4.0; native ORGAN/SAW, ROCK, FX, KEY ASSIGN LED, package CRC, protected boot bytes, WebInstaller embedded hash and cross-firmware gating all pass.
- `FM-1_2145` native JieLi pi32v2 binary and SHA256-pinned `.fwsc` with matching WebInstaller, one-command repeatable target build. **Real FM1 idle CPU percentage, hardware keyboard/LED electrical timing, hardware boot/OTA (historical -16), emulator note-halt and physical sound remain UNVERIFIED.** Do not infer a measured FM1 CPU percentage from host tests.

## V4.2 — October 19, 2026 (Main-loop IRQ Idle / zero audio concessions)

- **No audio-engine changes at all:** V4.1 `b1_engine.c`, `b1_engine.h`, performance renderer, FX, PCM arithmetic, 91 lazy-phase tonewheels, 12 voices, KEY ASSIGN LED policy, MIDI and storage remain unchanged.
- Replace the V4.1 main-loop 10ms tight busy spin with the native pi32v2 **`idle`** instruction. TIMER5 (10kHz), audio DMA and USB hardware interrupts remain enabled and wake the CPU.
- Reduce foreground watchdog feed, USB reconnect checks, and OTA identity-service polling from every busy-loop iteration to **once per actual millisecond**. Explicit bootloader/update requests and audio overruns bypass this throttle immediately. USB SIE polling and the 27-key/LED matrix scan remain in the original TIMER5 IRQ and are **not** rate-limited.
- Add `b1_cpu_idle.h`, pure scheduling tests for 10kHz-to-1kHz pacing, urgent flags, millisecond wrapping and actual pi32v2 opcode verification. Physical CPU-load percentage is **not** claimed from host-only checks.
- Native build `FM-1_2144`, pinned SHA256 WebInstaller and V4.1->V4.2 source-firmware allowlist. CRC, protected boot header, complete PCM regression tests and V4.0->V4.1 DSP exact-state differential remain checked.
- **Important:** this release addresses CPU0 main-loop spinning, NOT the 10kHz TIMER5 IRQ cost, DMA scheduling nor unverified hardware OTA error -16. Users should collect 10-second idle load (with USB attached/detached), full polyphony, USB MIDI, and recovery measurements on actual FM1 hardware before treating it as stable.
- No claim of hardware boot validation. Earlier emulator halt-on-note remains unexplained and may be unrelated.

## V4.1 — October 19, 2026 (Smart Idle Engine)

- Add a whole-block silence fast path in `b1_render_stereo` after **all** voice release, percussion, output-gate fade, delay/reverb tail and incremental FX ring-clearing work has finished. Stereo DMA still receives a fully written zero PCM buffer. No extra allocations, any latency wake-up or voice reductions.
- The 91 tonewheels remain **logically free-running**: their sample time advances with one frame-counter addition per silent block, and the existing exact lazy-phase reconstruction supplies the correct phase on the very first reactivated note. ORGAN/SAW crossfade and drawbar smoothing also advance mathematically/sample-identically during silence.
- The rare 32-bit frame-clock wrap takes the unchanged normal sample renderer, preserving wheel stamp clearing. During any release/FX tail or active note, the previous V4.0 audio path is unchanged.
- No rotor/scanner phase changes during fully gated silence: retains V4.0 phase semantics and PCM exactly. This release **does not** attempt new physical Leslie free-running behaviour as it would alter the existing sound.
- Native host differential test compares the **entire PCM stream and all DSP state bytes** against unmodified V4.0 across ORGAN/SAW, FULL/SAFE, long idle, live parameter changes, chord attacks, effect tails, and the wrap boundary. Legacy rock/percussion, LED and PCM golden tests remain intact.
- Host-only idle render microbenchmark: roughly 9.06 ns/sample for V4.0 vs 0.39 ns/sample for V4.1 on the test machine (single warm run, not physical FM1 CPU measurement). Playing-note CPU is not claimed to improve.
- Native AC791N pi32v2 `.fwsc` rebuilt as `FM-1_2143`, WebInstaller pins its exact SHA-256 and retains V4.0 (`FM-1_2142`) as an allowed upgrade source. Offline packaging does **not** prove physical flash/boot. Hardware test and historical OTA -16 remain outstanding.

## V4.0 — October 19, 2026 (KEY ASSIGN LED visibility)

- Based **directly on V3.9**, retaining its byte-for-byte untouched sound engine and all 91 tonewheels, 12 voices, drawbars, ROCK/LORD, B3/Leslie, ORGAN/SAW, effects, projects, MIDI and recovery safeguards.
- Active CHORD selector keys now stay **steadily illuminated** on the FM1's physical 27-key keyboard whenever CHORD ASSIGN is ON, including on other performance pages. All idle ORGAN or DISABLED keys stay dark, allowing immediate distinction between chord triggers and normal notes.
- LED ownership uses exactly the same effective, priority-ordered keyboard ranges as note-on routing. Only an *active* CHORD key lights: a saved chord quality on an ORGAN key is not enough; overlapping higher-priority ranges and ASSIGN OFF are respected.
- The dedicated SCALE page retains the original SCALE GUIDE, even when chord assignment is enabled. A pressed key still lights temporarily, so existing performance feedback is retained. When ASSIGN is OFF on the CHORD editor, unpressed keys are dark.
- The CHORD ASSIGN display now labels the LED convention `LIT=CHORD   DARK=KEYS`.
- New hardware-policy regression tests cover 27 physical note LEDs, 14 function LEDs, real CHORD range priority, toggling, saved/loaded assignments, scale-guide priority, pressed keys and legacy project formats.
- Target-compiled new `FM-1_2142` `.fwsc` and matching hash-pinned WebInstaller; actual FM1 hardware boot, LED scan, audio and OTA operation require user hardware testing. Do NOT infer real hardware functionality from emulation or offline tests.

# Changelog

Version history and technical notes for **SikkenWorks B1**, the drawbar-organ firmware for the M-VAVE FM-1.

This file contains release-specific changes, build validation, performance measurements and outstanding hardware-verification limits. For an overview of the instrument and its controls, see the [README](README.md).

---

## V3.9 — October 18, 2026 (bit-exact DSP efficiency)

- **No sound or functionality removed.** Retains the entire V3.8 engine, ROCK/LORD amp, B3, 91 tonewheels, 12 voices, every drawbar and mode, percussion, MIDI, UI, rotary, chorus, delay/reverb, project saving and BEETLE-safe flash guards.
- Replace four costly signed 64-bit multiply/divide pairs per sample in the **full ROCK/LORD filter path** with an exact 32-bit unsigned-magnitude Q15 helper. It follows signed truncation toward zero, including negative filter differences. Safe ROCK, B3, SAW/ORGAN, mode switching and all other DSP paths remain intact.
- Reduce the table softclip interpolator from two 32-bit multiplies to one using its monotone LUT property; preserve all old integer output values (exhaustive check of all 3 tables, all interpolation fractions). This also benefits the default SAFE ROCK path.
- Precompute ROCK tone, treble, presence and air coefficients once per `b1_render_stereo` call rather than inside the sample loop. Current hardware control values remain live at every new audio call (normally 128 frames).
- **Validation:** 2,500,090 randomized/edge fixed-point arithmetic comparisons plus 3,145,728 exhaustive LUT cases; 16 ORGAN/SAW/B3/ROCK/SAFE/FULL/Leslie/effect scenarios with 128 blocks each, **1,048,576 PCM bytes identical to V3.8**; existing native, OTA-package, installer, backward-compatibility, header and corruption tests. Actual FM1 CPU/headroom measurement and physical update remain unverified.
- New `FM-1_2139` target compile and `.fwsc`, with a WebInstaller embedding the exact byte-identical image and keeping V3.8 in the approved previous-firmware list. One-command `tools/build_v39_release.py` requires the user's installed toolchain; compiler/toolchain binaries are not redistributed.

## V3.8 — October 18, 2026 (upper percussion bus + rock tone)

- **Real UPPER percussion bus:** all currently held upper-manual notes feed a single decaying percussion envelope. Single-trigger behaviour, second/third harmonic selection, upper-only routing and automatic 1' drawbar cancellation are preserved. B3 single-note reference PCM is byte-identical to V3.7; chords intentionally sound different because additional upper notes now have the missing percussion tone. Lower and SOLO keys do not feed the upper percussion bus.
- **ROCK / LORD SAFE DSP:** replace the single symmetric speaker-saturation stage with one polarity-dependent nonlinear stage (different knees for positive/negative half-cycles), followed by a separate bass roll-off, upper-mid presence shaping and high-frequency ceiling. DRIVE still works even with CABINET MIX 0, while AMP GAIN, TONE, CABINET, scanner, Leslie, delay and reverb are preserved. ROCK samples intentionally differ from V3.7; clean/B3 remains unchanged except for the corrected polyphonic percussion.
- **Bounded performance:** cache the held UPPER keys' percussion tonewheel indices once per audio sub-block rather than checking 12 voice flags for each of 44,100 samples/second. No new heap allocation, no saved-project schema changes and no extra audio/FX buffers.
- **Native release:** freshly compiled with JieLi pi32v2 clang/ld, product `FM-1_2138`. The `.fwsc` matches the WebInstaller embedded image and pinned SHA-256. The versioned build script is rerunnable without losing its V3.7 upgrade source whitelist. Full source and previous functionality retained.
- **Tests:** rock/percussion bus, mono latch, held chord addition, lower exclusion, B3 V3.7 bit-exact reference, full 16-case ORGAN/SAW/FX stress, exact constant-division tests, native target build, bootheader, installer identity and corrupt-payload rejection. Physical FM1 boot, OTA compatibility and 60 MHz CPU margin **not yet measured**. Historical OTA error `-16` remains hardware-unverified.

## V3.7 — October 18, 2026 (experimental, target-compiled)

- Exact branchless-magnitude audio master gain for all 16-bit PCM values and 0–32767 levels, replacing signed division by 32767 per L/R sample. Values match old integer PCM bit-for-bit (tested).
- RGB565 LCD damage fingerprint reduced from two FNV multiplication steps per pixel to one, preserving bounded strips and full refresh every 64 frames.
- Prevent old-project MIDI note-on messages in the input queue from replaying during/after long project load/reset transactions.
- Native pi32v2 application target, firmware product `FM-1_2137`, regenerated `.fwsc`, built-in WebInstaller hash and byte-for-byte payload checks; still no physical FM1 verification.
- No changes to patch format, EEPROM/flash write boundaries, 91-tonewheel engine, 12 voices, sound modes, drawbars, or FX settings.



## V3.6 — Compiled FM1 update, nonblocking project transactions

### Realtime changes

- **Shorter interrupt masking on project changes.** A queued LOAD or NEW operation now runs after the UI event has released the global interrupt mask; only the small pause/resume flag updates are atomic. Previously resetting the 56 KB organ engine or applying a saved project happened while all hardware interrupts were disabled. The DMA ISR now writes silence to the currently CPU-owned half while project data is being replaced, without disabling the USB/timer interrupts, stopping ALNK0, or waiting through the 330 ms codec restart required for flash writes.
- **Consistent project saves.** Pause audio before snapshotting the project. This prevents an evolving audio/performance state from being captured across separate callback boundaries while a flash write is being prepared; the normal flash-write stop/restart protocol and two-sector journal remain unchanged.
- **More precise diagnostics.** Measure the longest interrupt-disabled UI event and display `UI … US` alongside the peak callback time in the SCL AUDIO DIAG screen. Short audio IRQ latency is more important than average desktop CPU time.
- Preserve 91 shared wheels, 12 voices, ORGAN and SAW modes, effects, MIDI, all controls, 12 project slots and partition protections. No engine sound-generation code was changed.

### Compiled release artifacts

- Built the complete native application using the user-supplied **JieLi pi32v2 clang/ld** toolchain (20250324.1 tool root from the 20250805.1 package); linked `B1.elf`, generated and audited `B1-app.bin`, and built `SikkenWorks-B1-V3.6-BEETLE-OTA.fwsc` (product `FM-1_2136`).
- Offline `fwsc` validation checks the protected 16 KiB BEETLE bootheader, OTA container, app entry, section layout, app CRC and package integrity. The WebInstaller embeds and SHA-256 pins this exact new firmware. The original protected bootheader bytes and BEETLE project storage exclusion have not been modified.
- New `tools/build_v36_release.py` builds the native ELF, app.bin, fwsc and WebInstaller, then runs static package audits in a single command. It requires `JIELI_TOOLCHAIN` in the environment; the toolchain itself is not redistributed.
- Passed native LCD damage tests, deterministic original ORGAN/SAW PCM golden tests, sixteen high-polyphony scenarios, exact fixed-point division/ramp comparisons, WebInstaller security/compatibility tests and the full native compiler+packer pipeline.

### Hardware verification limit

**This is a genuinely newly compiled, internally validated V3.6 `.fwsc`, not a renamed older image.** Successful host-native build and CRC/package checks cannot establish successful flashing, boot, sound or recoverability on a physical FM-1. The reported historical `OTA FAILED -16` remains unresolved by physical verification. Back up before installing and retain a proven independent recovery route.

## V3.5 — FM1 Open V12-informed realtime and LCD work (experimental source)

### Changed

- Compared the M-VAVE FM1 Open V12 Development Kit and public repo, notably the board audio IRQ and DMA buffer ownership logic. **The M-VAVE proprietary engine library is supplied as a binary, not complete engine source.** B1 retains its own Felucca HAL and original sound engines.
- Acknowledge the current ALNK half event before B1's PCM render; verify actual DMA half ownership using the double selector / new pending-flag check before and after committing output. Fail safely and latch low-cost DSP on a missed deadline.
- Track peak audio callback time in TIMER4 ticks; show microseconds on the AUDIO DIAG screen, refreshing diagnostics at 2 Hz.
- Avoid transferring unchanged LCD rows: FNV-1a RGB565 row hashes, bounded <=115-row SPI windows, <=2-row gap coalescing and periodic full refresh. No extra full-size framebuffer, preserving the same 240x240 UI rasterizer.
- Keep the original ORGAN/SAW engine, 91 wheels, all polyphony/FX, B1 project format and protected flash regions unchanged.

### Verification and limits

- New deterministic native LCD-damage test passed; native C syntax checks passed on the host.
- Historical test script passed the first 21 logged checks through V1.7 but did **not** complete before the execution timeout. Separately, V3.3 ORGAN/SAW and 16 dense-FX cases, the V3.0 exact comparisons and V3.1 division equivalence tests passed.
- This is a **source-only experimental build**. The pi32v2 toolchain and physical FM1 are unavailable here for target build, DMA measurements, or hardware OTA testing. **No V3.5 `.fwsc` or updated WebInstaller is included.** Any earlier V3.4 binaries included as references are not V3.5 firmware.
- A hardware run must measure UI-rate under load and DMA ownership transitions. The known earlier OTA FAILED -16 remains unverified on physical hardware.

## V3.4 — Realtime tonewheel and Leslie optimization

### Changed

- **Oscillator shaping:** moved three fixed, per-tonewheel ORGAN-character values into compact, build-generated ROM tables. The 91 shared tonewheels retain the exact V3.3 sine/partial arithmetic while avoiding repeated modulus calculations during active samples.
- **Polyphonic leakage:** determine each voice's leakage tonewheel once on note-on instead of repeatedly calculating modulo 91 inside the per-voice sample loop. One additional byte per transient voice; the saved project schema is unchanged.
- **SAFE-DSP Leslie:** replaced the two repeated signed `horn / 1100` divisions per sample with an exact, signed, 2,048-entry table derived from the existing sine oscillator ROM. No change to the horn waveform, pan or wet/dry arithmetic.
- ORGAN, frequency-banded SAWTOOTH, HOME + FX, twelve voices, drawbars, percussion, CHORD ATTACK, all effects, saved projects and UBOOT/OTA support remain available. No heap allocation has been introduced in the audio callback.

### Validation

- Eight deterministic **byte-identical** extended PCM stress renders compared against V3.3: full/SAFE DSP × ORGAN/SAW × B3/ROCK, with twelve notes, leakage, Leslie, chorus, delay, Cathedral-reverb, live engine switching and note release.
- Full historical DSP, headroom, chord, project-journal and scale regressions passed; all sixteen original SAW/polyphony stress scenarios passed.
- Nine-run desktop host benchmark (medians, same machine, 128-sample buffers) measured approximately **21–26% less computation** for four or twelve active voices in the original benchmark configuration. These figures do **not** establish realtime performance or DMA timing on the FM1.
- Native AC791N/pi32v2 firmware, standalone `.fwsc`, embedded WebInstaller and 67 genuine native RGB565 screenshots validated; the protected BEETLE bootheader was retained byte-for-byte.

### Known limits

- V3.4 has **not** been physically installed or measured on the 60MHz M-VAVE FM1. Reduced host computation cannot guarantee crackle-free audio or resolve the previously reported on-device `OTA FAILED -16` issue.
- External custom firmware installation continues to depend on compatible bootloaders and each firmware's own installer; B1 cannot promise universal flashing or recovery.

## V3.3 — Experimental ORGAN / SAWTOOTH engine

### Changed

- Added a genuine **ENGINE MODE** for all **91 shared tonewheels**: **SINE / ORGAN** retains the original tonewheel calculation; **SAWTOOTH** replaces the oscillator waveform without adding a second synthesizer or extra voices.
- **HOME + FX** together opens the native mode screen. K1 selects ORGAN or SAWTOOTH; **SEL** also toggles the mode and **HOME** returns to the normal drawbar page. A short key-combination grace period prevents accidental standalone HOME/FX actions.
- Added eight precomputed, **512-sample band-limited saw tables**, selected by tonewheel frequency to limit aliasing. No Fourier transforms, trigonometry, heap allocation or additional voices are required in the audio thread.
- Added an approximately **6 ms crossfade** between engines, including while notes are held.
- Saved the engine choice in a previously unused, checksum-covered byte of the existing **V6 project record**. The record remains **124 bytes** with its checksum at the same offset. New and older projects default to ORGAN.
- Retained all nine drawbars per registration, 12-voice polyphony, percussion, chords, Leslie, chorus, delay, reverb, startup animation and cross-firmware/UBOOT controls.

### Validation

- Two reproducible, byte-identical native builds; firmware ID **`FM-1_2133`** and checked WebInstaller image.
- Six ORGAN PCM references were byte-identical to V3.2. SAW/ORGAN passed 16 demanding polyphony/FX host scenarios in full and light DSP with B3 and ROCK processing.
- Mode switching, HOME+FX detection, project save/load and legacy project-default behavior passed native tests.
- **67 native 240 × 240 screen/boot captures** passed pixel-for-pixel verification, including both ENGINE MODE screens; 240 × 240, 960 × 960 and 1920 × 1920 PNG collections were provided.

### Known limits

- SAW mode and OTA installation have **not been tested on the physical FM-1**. The previous `OTA FAILED -16` hardware issue has not been proven fixed.
- The band-limited waveforms reduce avoidable aliasing; they do not guarantee a noise-free analog output or no realtime buffer underruns.
- Projects saved by V3.3 are not guaranteed to be readable by older B1 releases.

## V3.2 — Cross-firmware bridge and loader verification

### Changed

- Replaced the FM-1-side OTA loader SHA-256 decision with an **exact byte-for-byte comparison** against the pinned BEETLE RC1.01 update-loader bytes, without allowing an unknown loader through.
- Distinguished **`-16`** (loader mismatch before staging) from **`-18`** (NOR-flash staged readback mismatch).
- Added an **OTHER CUSTOM .FWSC** route to the WebInstaller for FM-1 images with the exact compatible protected boot header and loader. UFW structure, CRCs, image bounds and loader identity are checked before writing.
- Expanded **ENTER UBOOT** handoff: the installer waits for a responding, compatible USB-MIDI update loader before enabling an installation. For other bootloader protocols or layouts, use the target firmware's original installer.

### Validation

- Two identical native JieLi builds, package CRC and 16 KiB BEETLE header checks, pinned loader tests and installer SHA-256 checks passed.
- Known B1/BEETLE files passed compatibility checks; deliberately corrupted or truncated UFW files were rejected.
- Historical organ, chords, effects and storage tests and **65 native screen captures** passed.

### Known limits

- A new firmware cannot retroactively change a defective OTA verifier **already running** on the FM-1; the UBOOT alternative requires a working device bootloader.
- The original `-16` issue, physical UBOOT re-enumeration, third-party installer compatibility and recovery have **not been validated on hardware**. Arbitrary firmware layouts are deliberately not flashed blindly.

## V3.1 — Reverb and amplifier DSP pass

### Changed

- Replaced three variable reverb damping divisions per audio sample with a **block-level Q16 reciprocal**, multiplication and bounded integer correction, preserving signed rounding behavior.
- Simplified the ROCK/LORD amplifier sag calculation by selecting its two fixed divisors directly.
- Kept project schema, UI, tonewheels, effects, startup, UBOOT and protected BEETLE boot header unchanged.
- Updated native firmware and the WebInstaller to **`FM-1_2131`**. A later installer-only repair corrected recognition of some stock and OTA-loader device identities after users encountered `Unsupported firmware`.

### Validation

- Two byte-identical native builds; six PCM files and four additional full/light-DSP FX references matched V3.0 byte for byte.
- 250,000 signed arithmetic comparisons, heavy 12-voice effects, CHORD ATTACK, project tests and **65 native UI captures** passed.

### Known limits

- The on-device `OTA FAILED -16` reported during V3.1 installation was **not resolved merely by the installer identity repair**. Actual FM-1 audio-interrupt headroom and cross-firmware recovery were not measured.

## V3.0 — Gain and ramp optimization

### Changed

- Avoided unnecessary runtime division for small Leslie/drawbar ramp changes while keeping the original ramp results.
- Simplified full-range signed-magnitude calculations in the gain/headroom helpers to avoid expensive 64-bit arithmetic where not required.
- Updated firmware identity to **`FM-1_2130`**, retaining the existing WebInstaller and BEETLE/UBOOT structure.
- Kept the project format, audio controls, voice count and UI layout unchanged.

### Validation

- Two byte-identical JieLi builds and **250,000 arithmetic-equivalence comparisons** passed.
- Four long-form PCM references were byte-identical to V2.9.
- **65 native 240 × 240 captures** passed. Host benchmarks showed workload-dependent improvements, not one universal speedup figure.

### Known limits

- Host timings do not establish audio-DMA stability or crackle-free polyphony on a physical 60 MHz FM-1.

## V2.9 — Realtime processing and external-installer handoff

### Changed

- Moved scanner, rotary, delay and reverb coefficients that stay constant within a buffer **out of the per-sample path**.
- Simplified fixed-point gain calculations while retaining the previous signal and saturation behavior.
- Added **ENTER UBOOT / EXTERNAL INSTALLER** to the WebInstaller, requesting the existing FM-1 update mode without first writing a firmware file. Preserved OCT− at boot and REC safe-mode entry points.
- Updated firmware identity to **`FM-1_2129`**. Compatible B1/BEETLE-style packages remain checked before OTA; alternative loaders require their own compatible update procedure.

### Validation

- Seven base WAV comparisons and four longer 12-voice, all-FX PCM comparisons were byte-identical to V2.8.
- Native build reproducibility, source/installer SHA-256, BEETLE header and **65 UI screenshots** passed.

### Known limits

- An UBOOT command being sent is **not proof** that a bootloader becomes available or accepts any specific third-party installer. Actual FM-1 firmware switching was not validated.

## V2.8 — CHORD ATTACK timing and audio optimization

### Changed

- Corrected CHORD ASSIGN monitoring: the old page suppressed the physical key state delivered to audio, so the selected chord could not be heard while editing.
- **CHORD ATTACK** now spans simultaneous onset at **0** through approximately **65 ms** of staggered, randomized note order at **100**, with sample-based scheduling, reliable note-off and cancellation.
- Skipped unnecessary tonewheel contact calculations when **KEY CLICK = 0** and optimized a bounded scanner interpolation operation.
- Retained project schema V6, effects, keyboard ranges, 12 voices and the native startup.

### Validation

- New tests covered attack extremes, randomized timing, playing chords from the editor, note-off and project persistence.
- Existing native tests, the installed-image SHA checks and **65 code-rendered UI captures** passed.
- Two reproducible FM-1 builds were generated.

### Known limits

- Reduced host DSP work does not prove that remaining FM-1 crackling or DMA underruns are fixed on hardware.

## V2.7 — CHORD ASSIGN range and ON/OFF controls

### Changed

- Corrected `KEY OUTSIDE CHORD RANGE` for new projects: CHORD ASSIGN now has an explicit **K3 ON/OFF** control, defaulting to OFF. When enabled, a suitable unused range can become a chord-selector zone on **keys 21–27**, without overwriting existing ORGAN ranges.
- **K1** browses keys, **K2** selects the chord quality, **K3** toggles ASSIGN and **K4** adjusts CHORD ATTACK (0–100). Press a real key in the active selector range to assign the chosen chord.
- Disabling the chord selector returns normal keyboard behavior without deleting assignments. The enabled state persists in an unused V6 project byte; the 124-byte project size and checksum position stay unchanged.

### Validation

- Native assignment, keyboard-role, ON/OFF, attack, backward-compatible project save/load and historical regressions passed.
- Two byte-identical native builds, installer/firmware hash and **64 native UI images** passed.

### Known limits

- Physical FM-1 operation and recovery were not confirmed by the offline suite.

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

For the current **V3.3** package:

- `build/SikkenWorks-B1-V3.3-BEETLE-OTA.fwsc` is an offline-checked JieLi application-update candidate with product ID `FM-1_2133`, based on the protected BEETLE RC1.01 header reference. Physical installation remains **unverified**.
- `B1-WebInstaller.html` embeds the same SHA-256-checked firmware image. Compatible FM-1 images can be validated against the exact recognized loader/boot layout; external images requiring another loader must use a verified UBOOT/vendor route.
- `build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc` is the unchanged BEETLE reference file, **not** a universal or guaranteed recovery method.
- Full C source, versioned test reports, build tools, native UI captures, organ/saw PCM examples, licenses and attribution are included in the release.
- The previous on-device `OTA FAILED -16` report is not physically verified as resolved. Do not confuse a valid update package with successful bootloader handoff or flash execution on actual hardware.

Neither an offline `.fwsc` check nor a C-engine audio reference establishes physical FM-1 boot success, DAC integrity, realtime DMA scheduling margin or safe rollback. Preserve an independently tested recovery method.
