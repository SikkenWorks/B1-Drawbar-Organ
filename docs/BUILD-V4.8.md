# Build B1 V4.8

Native FM1 target: JieLi pi32v2 (AC791N/WL82). Building never flashes hardware.

```bash
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1
python3 tools/build_v48_release.py
python3 tools/verify_v48_release.py
```

This builds `build/SikkenWorks-B1-V4.8.fwsc` and `B1-WebInstaller.html`, and runs native C OTA replay and host PCM tests. For the reference loader/build header, keep the original BEETLE RC1.01 reference package included under `build/` exactly unchanged. The packer SHA-pins the reference. The executable **application identity and package product** are **FM-1_481026**; the historical loader/reference is not B1's identity.

For a real FM1 update, first make a verified independent backup and test the rescue path. Software-only tests cannot establish safe OTA boot or click-free 3+ voice rendering on 60 MHz hardware.
