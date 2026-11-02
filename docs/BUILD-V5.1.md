# Build B1 V5.1 (FM-1_481026)

The complete native source tree is supplied. The toolchain archive is **not** copied into this ZIP; install the separately licensed JieLi pi32v2/AC791N toolchain. Use the existing reference firmware fixture as-is, do not alter its protected boot bytes.

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1
python3 tools/build_v51_release.py
python3 tests/test_v51_release.py
python3 tests/test_v51_lossless.py
```

Output: `build/SikkenWorks-B1-V5.1.fwsc` and `B1-WebInstaller.html`. The native compiler targets `pi32v2`, and the packer checks original flash layout, app bounds, CRC and `FM-1_481026` before emitting a UFW firmware. Source ZIP includes original build scripts, pinned original boot-reference fixture, LGPL/GPL or other applicable upstream notices from the source, and historical tests.

The inclusion of a compileable firmware is not evidence of successful physical USB-OTA firmware switching. Hardware flash only with independent recovery/backup and validated source firmware.
