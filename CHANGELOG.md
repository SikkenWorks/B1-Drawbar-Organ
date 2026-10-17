# B1 V2.5 — Audio overload / clean output

- Avoid accidental signal hardening from multiple unconditional soft-saturators.
- Add a fast-attack, slow-release preamp input guard and a stereo-linked
  output ceiling with extra master digital headroom.
- Preserve intentional ROCK/LORD drive and the existing factory rotor defaults.
- Leave storage, OTA, MIDI, project serialization, UI and Felucca HAL unchanged.
- Reproduce the FM1 app build, check package SHA-256/CRC and regenerate 62 native UI captures.
- Physical confirmation of crackle/dropout repair is still required.

# Changelog

## B1 V2.4 — October 17, 2026

- New/Reset Project: Leslie rotor defaults to FAST and ROTOR MIX 60.
- Set rotor phase increments to FAST upon reset.
- Old saved projects retain their exact stored values.
- Same 240x240 firmware renderer regenerated 62 PNG screenshots.
- New verified installer, OTA package and separate screenshots-only ZIP.

## B1 V2.3 — October 17, 2026

- Scanner chorus and vibrato use interpolated fractional delay in FULL and SAFE DSP modes; smooth bypass transitions.
- Added native delay with MIX, TIME, FEEDBACK and STEREO.
- Added native SPRING, ROOM, HALL, CATHEDRAL reverb modes with MIX, DECAY and DAMP.
- Added 2 new FX pages (now 6/6).
- Fixed PROJECT MENU footer to match physical SEL input (not ALG), saved and loaded confirmation shown for 1600 ms.
- Project schema V6 stores every sound / range / harmony / effect control including global octave and selected bank.
- Backward-compatible read and journal migration from V2.2 V5 project archives.
- Added bounded, wet-only FX tails with no unkeyed oscillator feed; incremental idle ring clearing.
- Retained all existing B1 audio, organ, key, chord, MIDI, hardware, startup and firmware update code.

## Safety

OTA/USB-header offline parity is checked against the supplied BEETLE reference. The physical FM1 update and audio have **not been validated** in this execution environment.
