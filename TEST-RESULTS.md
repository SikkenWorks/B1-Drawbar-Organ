# B1 V2.3 test results

## Native FM1 build

- Target: JieLi pi32v2, `firmware/build.py`.
- App entry ELF to FM1 app conversion passed address/load section audits.
- Two application builds were byte-identical.
- `.fwsc`: CRC and embedded application audit passed. Protected first 16 KiB is byte-identical to original supplied BEETLE 1.01 RC1 package.
- WebInstaller contains byte-identical firmware; SHA-256 binding, firmware identity, tampering refusal, and BEETLE restore tests passed.
- Boot/flash/rollback on physical FM1: NOT tested.

## Regression

- `bash tests/run_v03_tests.sh`: PASS, including all legacy functional tests; 62/62 native RGB565 frames match exported PNGs pixel by pixel.
- `node tests/test_installer.cjs`: PASS.
- `native_v23_fx_project`: PASS full/safe DSP scanner V1/V3/C2, delay and four individually different reverb outputs, all project parameters roundtrip.
- `native_v23_v5_migration`: PASS V2.2 schema-1 journal -> valid V6 record, new schema-2 journal commit and simulated reboot.
- `native_v23_max_fx_stress`: PASS in both DSP paths with 12 voices, all nine drawbars at 8, max chorus, Leslie, delay and Cathedral reverb.
- AddressSanitizer and UndefinedBehaviorSanitizer of both FX and archive migration test cases: PASS.
- Recorded 7 reference WAVs using `firmware/b1_engine.c`. No physical-device recording or listening equivalence claimed.

## Remaining hardware verification

- Clean chorus/vibrato with live encoders and modulation depth changes.
- Reverb and delay on 60MHz target, especially 12-voice overdrive; no guaranteed realtime CPU budget yet.
- Persistent project save, power-off and load including V2.2 project migration.
- SEL button behavior and 1.6 s save/load confirmations on the physical FM1.
- Firmware flashing, switching to BEETLE/stock, and independent recovery.
