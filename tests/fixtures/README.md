# Native OTA regression fixture

`v54-short-app-valid.ufw` is a **synthetic** CHECK fixture derived from the V5.4 firmware image. Its encrypted `app.bin` directory declares a shorter app slot and has a matching recomputed app CRC and UFW CRCs. The original protected boot bytes and loader have not been changed. It is **not a vendor firmware, not a Felucca release, not a stock firmware and NOT intended for flashing**. Only the host replay test may use it.

The original unmodified B1 V5.4 and original BEETLE RC1.01 binaries are tested separately from `build/`.
