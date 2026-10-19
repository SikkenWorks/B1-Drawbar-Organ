# B1 V4.3 WebInstaller HF2 — SLOOP/Felucca source transition

This is an **installer-only hotfix**. The B1 V4.3 FM1 firmware is not modified or rebuilt.

## Root cause (verified by source comparison)

1. HF1 rejected source `FM-1_900` at CONNECT unless two Advanced Mode checkboxes were enabled. SLOOP reports `FM-1_900` but this identity is shared by numerous Felucca-derived firmwares. It is not a source-protocol capability declaration.
2. HF1 waited **only** for `ota-FM-1_015` in the bootloader stage. The published SLOOP updater uses `ota-FM-1_9xx` (in particular `ota-FM-1_900` for SLOOP 2.6). So fixing CONNECT alone was insufficient.

## Fix

- Normal CONNECT accepts a well-formed `FM-1_NNN` application identity from an FM-1 device; **Advanced Mode is not needed when B1 is the target**.
- Before attempting a cross-firmware installation, an explicit confirmation explains that running CHECK can switch firmware into an OTA loader.
- CHECK only completes if the FM1 speaks the expected two-stage SysEx read-request protocol, requests the verified firmware image and sends the expected completion message.
- After CHECK, only `ota-FM-1_015` (existing path) or `ota-FM-1_9xx` (SLOOP/Felucca family) are permitted for WRITE; any other loader halts.
- The original package SHA-256, target product identity, protected boot-header and loader audits are unchanged.
- Advanced Mode stays available only for explicitly approved *target* restore images; switching package selections does not reset it.
- Other source firmware identities are not automatically trusted: compatibility remains subject to protocol stage and loader verification. This is **not** an unconditional promise of installing from every possible firmware.

## Offline verification

Run from the extracted ZIP if Node.js is available:

```sh
node tests/test_transition_hf2.cjs
```

Tests simulate connected SLOOP `FM-1_900` without Advanced Mode, the successful CHECK/LOADER/WRITE/REBOOT sequence using approved `ota-FM-1_900`, `ota-FM-1_906`, and `ota-FM-1_015` loaders, and refusal of unsupported loader IDs before WRITE. Original embedded firmware bytes and SHA-256 are tested against the standalone package. **These tests do not open any MIDI ports, flash a device or prove the physical loader behavior.**

## Before a real SLOOP → B1 update

- Back up SLOOP projects, user presets, samples, and settings using SLOOP's original editor before changing firmware. B1 cannot open SLOOP project formats.
- Confirm you can access SLOOP's own USB RESCUE mode: power on while holding OCT−. This does not guarantee a recovery from every possible failure. If there is no functional bootloader, specialist FM-1-transporter recovery may be necessary.
- Open HF2 from HTTPS/localhost in desktop Chrome or Edge; use a USB data cable directly connected, and close other MIDI applications.
- Select **NATIVE B1 FIRMWARE** → **CONNECT FM1** (no Advanced Mode required) → read the `FM-1_900` identification and warnings → confirm backups and install.
- Do not disconnect USB during CHECK, LOADER or FLASH. If it stops at CHECK or LOADER, **do not retry blindly**: save the diagnostic log, take note of the exact device display error, and use the original SLOOP documentation to recover safely.

## Provenance

SLOOP upstream (GPL-3.0): https://github.com/isod89/sloop-fm1

SLOOP's `tools/fm1_install.py` documents identity, two-stage OTA, and the `ota-FM-1_9xx` loader family. Felucca/Hügelton Instruments' GPL-3.0 origin must be preserved. Existing B1 credits and licences remain included.

## Limits

- No physical FM1 test carried out in this environment.
- Whether SLOOP's CHECK stage accepts the V4.3 B1 image on the user's hardware is currently **unverified**.
- The earlier B1 `OTA FAILED -16` issue is not proven fixed.
- A firmware using a different USB protocol, incompatible loader, or inaccessible USB interface cannot be universally reflashed by this browser page. Use its documented updater or a proven hardware recovery route.
