# Build B1 V5.2 (permanent identity FM-1_481026)

Install the separately licensed JieLi pi32v2 toolchain; the full source ZIP does not redistribute toolchain binaries.

```sh
export JIELI_TOOLCHAIN=/absolute/path/to/jieli-linux-toolchains-20250324.1
python3 tools/build_v52_release.py
python3 tests/test_v52_release.py
python3 tests/test_v52_lossless.py
cc -O2 -std=c11 -Ifirmware tests/test_v52_arithmetic.c -o /tmp/b1-v52-arithmetic
/tmp/b1-v52-arithmetic
```

Output: `build/SikkenWorks-B1-V5.2.fwsc`, `B1-WebInstaller.html`, `build/B1.elf` and `build/B1-app.bin`. The packer preserves validated original boot/OTA bytes and enforces app size and package identity. The included BEETLE reference is a build fixture, **not** this firmware's product identity.

Do not mistake host PCM equality for physical timing. Hardware testing and safe independent recovery are necessary.
