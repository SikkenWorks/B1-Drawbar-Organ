# B1 V5.0 — native build

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1
python3 tools/build_v50_release.py
python3 tests/test_v50_release.py
```

Produces `build/SikkenWorks-B1-V5.0.fwsc` and `B1-WebInstaller.html`. Application/product identity is permanently `FM-1_481026`. Full code and firmware build are provided; OTA and recovery are unchanged from V4.9. Refer to `V5.0-AUDIO-PERFORMANCE-VERIFICATION.md` for constraints and hardware testing.
