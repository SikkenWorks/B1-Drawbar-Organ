# B1 WebInstaller HF1 — V4.2 / V4.3

This is an **installer-only hotfix**, not a new DSP firmware release. The organ engine and both .fwsc images are unchanged.

## Problems fixed

- Clicking firmware selections no longer clears **Advanced Mode**. The separate advanced source compatibility checkbox also stays checked while choosing another target, until the user disables the mode or reloads the page.
- Firmware Freedom restore is explicitly selected with **SELECT AN APPROVED RESTORE TARGET**, rather than hijacking the general Advanced Mode checkbox.
- The firmware identifier `FM-1_900` is shared by numerous independent firmware projects. It is not proof of a particular source firmware or OTA loader. The new, **off-by-default** FM-1_900 compatibility checkbox is available under Advanced Mode. A user must affirm compatibility and acknowledge the risks before a native B1 update can be attempted.
- Firmware hash, product identity, protected boot header, and OTA loader checks remain. A normal CHECK response and transition to the expected `ota-FM-1_015` loader are still required; other loaders cannot be written through this installer. The firmware remains fully protected by device-side validation.
- Unknown identities other than FM-1_900 remain blocked for app-OTA. The independent USB rescue and source firmware's own original updater remain the fallback.

## Safety and limitations

FM-1_900 is **not uniquely attributable** to Felucca, SLOOP or any other firmware. Some firmware may lack the compatible two-stage protocol, and an attempted update can fail, erase incompatible project data or require USB recovery. The browser cannot establish the current flash layout or recovery capability from the identity reply alone.

**No actual FM-1 hardware has been connected or flashed in this environment.** Do not assume a USB/OTA error (`-16`) has been resolved. Back up with the source firmware's tools, stop playback, ensure a tested recovery path and use a supported desktop Web MIDI browser. If a firmware check fails, do not repeatedly attempt flashing; inspect the downloaded log.

The pinned firmware identity and hashes have deliberately NOT changed:

- V4.2: `FM-1_2144`
- V4.3: `FM-1_2145`

Use the matching hotfix WebInstaller for the desired existing firmware.

## Tests

- Node syntax check passed on extracted main JavaScript for both HTML files.
- Isolated JavaScript UI tests passed: Advanced Mode persists during native/generic/stock/restore selection; `FM-1_900` needs explicit authorization; an unauthorized source cannot start an update. No real MIDI was sent.
- Both embedded .fwsc blobs are **byte-identical** to the supplied original files.
- The existing audio source and FWSC have not been changed.
