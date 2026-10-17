# SikkenWorks B1 V1.7 — Credits & license

SikkenWorks B1, © 2026 SikkenWorks.

## Felucca — required upstream attribution

**Felucca by Leo Kuroshita (@kurogedelic), Hügelton Instruments**

- Source: https://github.com/hugelton/Felucca
- License: **GPL-3.0-only**
- Used in B1: FM1 device startup/linker integration, hardware abstraction, LCD, USB-MIDI, interrupt system and recovery-related platform support (based on Felucca source in the previous SikkenWorks ETCH/B1 chain).
- The derivative B1 firmware is distributed **GPL-3.0-only**, preserving the upstream license, notices, and full corresponding source where reused. License text is included as `LICENSE`; original third-party license text is also included in `firmware/third_party/FELUCCA-GPL-3.0.txt`.

Felucca attribution must remain visible in project documentation and the B1 Safe Mode firmware screen.

## SikkenWorks ETCH and BEETLE

- SikkenWorks ETCH: https://github.com/SikkenWorks/ETCH. Existing FM1 development research, low-level firmware assets, native build infrastructure and range/keyboard design are technical reference points.
- SikkenWorks BEETLE: https://github.com/SikkenWorks/BEETLE. Project menu behavior and the FM1 currently installed with BEETLE are compatibility requirements. An exact installed BEETLE firmware/header match and a verified stock restore route have **not** yet been established.

## SikkenWorks B1 original code

The B1-specific additive tonewheel organ core, rotary/chorus DSP, chord performance logic, drawbar UI and RGB565 image extraction pipeline are original SikkenWorks implementation; they are shipped as part of the above GPL-3.0-only firmware source package.

## Status

B1 V1.7 includes a real, checksum-checked `.fwsc` OTA candidate with the protected BEETLE RC1.01 boot header and matching WebInstaller. Physical flashing, audio realtime performance, and restoration after failure have **not** been proven.

B1 reuses GPL-3.0-only OTA validation code in `firmware/ota.c` and `firmware/native_digest.c`, based on Felucca by Leo Kuroshita / Hügelton Instruments.

## UI font attribution

On-device bitmap glyphs compiled into `firmware/b1_font.h` were rasterized from Inter by The Inter Project Authors (© 2016–2023) under SIL Open Font License 1.1. See `firmware/third_party/INTER-OFL-1.1.txt`. No external font files are shipped.

## B1 V1.3 particle boot glyphs

The native startup screen uses static 2-bit bitmap glyph masks rasterized from Inter (Inter Project Authors, SIL OFL 1.1) in `firmware/b1_startup_masks.h`. The code contains the glyph pixels, not downloadable font files. `firmware/third_party/INTER-OFL-1.1.txt` contains the license.
