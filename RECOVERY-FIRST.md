# B1 V2.1 — Recovery and storage precautions

1. Back up BEETLE / ETCH and B1 projects and keep a known-good stock or BEETLE firmware image before replacing firmware.
2. Confirm your **own** hardware recovery method still works. Hold OCT− while switching on to request the preserved UBOOT update route; REC at startup requests Safe Mode without project scan.
3. The WebInstaller verifies firmware SHA-256 and the protected boot header; it does not bypass unknown firmware safety checks. Flashing is not risk-free and arbitrary third-party update tools may reject firmware identifiers.
4. When prompted about `UNKNOWN FLASH DATA`, choose REC or HOME for **NO** unless you have backups. YES requires SAVE twice, and may irreversibly erase two 4 KiB sectors from `0xE7000..0xEFFFF`. It never erases arbitrary foreign partitions.
5. If the FM1 stops responding to normal MIDI update commands, do not blindly repeat flashing. Use your already proven bootloader/recovery procedure and retain a full flash backup.
6. This build has not been validated on physical hardware and cannot promise compatibility with all installer versions, firmware families or loader variants.
