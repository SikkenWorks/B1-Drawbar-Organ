# Building and verifying B1 V5.7

The compiler is not included in the release. Use the JieLi pi32v2 Linux toolchain you provided, or obtain a compatible one separately.

```sh
export JIELI_TOOLCHAIN=/absolute/path/to/jieli-linux-toolchains-20250324.1
python3 tools/build_v57_release.py
python3 tests/test_v57_release.py
```

## Artifacts

- `build/SikkenWorks-B1-V5.7.fwsc` (real AC791N OTA app candidate)
- `build/B1.elf`, `build/B1-app.bin`
- `B1-WebInstaller.html` (embedded exact FWSC, SHA-256 pinned)
- `fixtures/original_MVAVE_Open_V15.fm1.fwsc` (original FM1 Open SDK V15; test fixture, not automatically flashed)

Build source and product identity are always `FM-1_481026`. Do NOT substitute BEETLE product ID.

## Tested / not tested

Native OTA, JEDEC allowlist and old V15 vs V16 boot gates are tested with C host replay. DSP regression checks compare the full engine state and PCM, but the V5.7 engine source itself is unchanged from V5.2. No hardware CPU profile, physical audio output, V16 host update or real cross-firmware roundtrip is claimed.

### Error codes

- `-16`: executable loader differs from the exact pinned trusted loader (use independent UBOOT)
- `-26`: unknown/undetectable SPI NOR ID (no B1 flash writes)
- `-27`: full-flash V15 restore blocked on UCUN (use verified V16-aware external recovery)
- `-28`: eight-entry vendor package is not authenticated official V15
- `-29`: installed protected 16 KiB boot area differs from the audited reference

Do not use a successful test log alone as permission to flash. Protect your flash backup and use a verified physical recovery method.
