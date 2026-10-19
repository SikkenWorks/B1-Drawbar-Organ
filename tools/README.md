# Build and test

**Current release:** `build_v37_release.py`. It compiles the native JieLi application, builds the FM1 OTA package, updates the inlined WebInstaller firmware, and checks integrity. Older scripts are retained for historical reproducibility only.

Required: JieLi pi32v2 Linux toolchain, Python 3, GCC/Clang for host tests, and Node.js for installer tests. The toolchain is not redistributed.

```sh
JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1 python3 tools/build_v37_release.py
sh tests/run_v37_quick_tests.sh
node tests/test_installer_v37.cjs
node tests/test_v37_cross_firmware.cjs
```

The `.fwsc` is offline-verified, not physically tested. Real FM1 boot, audio, OTA migration and independent recovery need hardware validation.
