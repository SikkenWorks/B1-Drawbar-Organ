# B1 V2.4 verification

- Factory RESET: rotor_fast = 1 / rotor_mix = 60 / rotor_enabled = 1.
- FAST rotor and drum increments on factory reset; old saved project rotor parameters preserved after project apply.
- New focused default-and-project-preservation native test: PASS.
- V2.3 regression suite (host DSP/keys/chords/storage/UI): PASS.
- 62/62 frames pixel identical between RGB565 and PNG: PASS.
- JieLi pi32v2 cross-compile, ELF layout audit, FWSC CRC: PASS.
- Two independent application builds and firmware-package builds byte-identical: PASS.
- WebInstaller embedded firmware exact SHA-256; BEETLE reference and tamper checks: PASS.
- Package identity: FM-1_2124.
- Firmware SHA-256: `56968a82522113852e91831be2dfefd94c438adc785b42a0675c886e5edc7a3a`.
- FM1 physical audio, flash and return-to-stock not tested.
