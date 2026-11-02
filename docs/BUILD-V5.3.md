# Native B1 V5.3 build and verification

Install the separately distributed JieLi toolchain and set `JIELI_TOOLCHAIN` to its root.

```sh
export JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchains-20250324.1
python3 tools/build_v53_release.py
python3 tests/test_v53_release.py
```

Native host tests:

```sh
cc -O2 -std=c11 -DB1_STORAGE_DYNAMIC=1 -Ifirmware tests/native_v53_autosave_switch.c firmware/b1_storage.c firmware/b1_performance.c firmware/b1_engine.c -o /tmp/b1-v53-storage
/tmp/b1-v53-storage
cc -O2 -std=c11 -Ifirmware tests/native_v53_ui_toggle.c firmware/b1_ui.c firmware/b1_performance.c firmware/b1_engine.c -o /tmp/b1-v53-ui
/tmp/b1-v53-ui
```

The old schema-3 archive is unmodified. The ON/OFF preference occupies the journal's previously unused header bytes 28..31. The recognized ON token is 0xA5073AD5; every unknown or old header value means OFF. Recovery mode does not scan or write the journal. Autosave changes require silence; a power cut before commit leaves the last choice in place. A hardware FM1 test is needed to confirm flash timing and display/control feel.
