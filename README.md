<div align="center">

<img src="B1.gif" alt="B1 drawbar organ on the M-VAVE FM-1" width="240">
<br>
# B1 Organ
<br>
**A drawbar organ for the M-VAVE FM-1.**

Tonewheels, nine drawbars, rotary speaker, percussion and the character of a classic rock organ, built into a pocket-sized instrument.

[The instrument](#the-instrument) &nbsp;·&nbsp; [How to use it](#how-to-use-it) &nbsp;·&nbsp; [Effects](#effects) &nbsp;·&nbsp; [Projects](#projects) &nbsp;·&nbsp; [Installation](#installation) &nbsp;·&nbsp; [Credits](#credits) &nbsp;·&nbsp; [Changelog](CHANGELOG.md)

</div>

---

## The instrument

B1 is open-source organ firmware by **SikkenWorks** for the **M-VAVE FM-1**. Rather than playing back an organ recording, it builds the sound from tonewheel-style harmonics that you mix with drawbars while you play.

Two organ characters are available: **B3 / LESLIE** for classic tonewheel sounds and **ROCK / LORD** for a more aggressive, driven rock-organ sound. Both can be shaped with percussion, key click, leakage, chorus/vibrato and a rotary-speaker effect.

| | |
| :-- | :-- |
| **Sound** | Nine-drawbar, tonewheel-style synthesis |
| **Organ characters** | B3 / LESLIE and ROCK / LORD |
| **Polyphony** | Up to 12 simultaneous voices |
| **Registrations** | UPPER, LOWER and SOLO |
| **Performance** | Keyboard ranges, chords, attack timing, scales and MIDI routing |
| **Effects** | Scanner chorus/vibrato, rotary speaker, delay and reverb |
| **Memory** | 12 named, persistent projects |

## How to use it

### 01 / Shape the organ with drawbars

Each drawbar controls a different harmonic. Pull it out to make that part of the sound louder; push it in to remove it. The nine drawbars follow the familiar tonewheel-organ order:

| Drawbar | Harmonic character |
| :-- | :-- |
| `16′` | Deep, one octave below the fundamental |
| `5⅓′` | Fifth above the 8′ fundamental |
| `8′` | Fundamental note |
| `4′` | One octave above |
| `2⅔′` | One octave and a fifth above |
| `2′` | Two octaves above |
| `1⅗′` | Two octaves and a major third above |
| `1⅓′` | Two octaves and a fifth above |
| `1′` | Three octaves above |

The drawbars use the usual **0–8** registration scale: `0` is silent and `8` is fully open. They respond smoothly to changes, including when notes are held.

B1 has three separate registrations: **UPPER**, **LOWER** and **SOLO**. Each has its own nine-drawbar setting. Use the organ and range pages to assign the sounds across the keyboard.

### 02 / Choose the character

**B3 / LESLIE** is the cleaner, classic organ path. Shape the sound with percussion, key click and tonewheel leakage, then add scanner chorus/vibrato and rotary-speaker movement.

**ROCK / LORD** adds an amplifier character with adjustable **AMP GAIN / DRIVE**. Low settings remain more restrained; higher settings deliberately add saturation.

Other organ controls include:

- **Percussion:** the familiar harmonic attack, with third harmonic enabled by default on a new or reset project.
- **Key click and leakage:** adjust the mechanical character of the instrument.
- **Scanner chorus/vibrato:** C/V modulation for classic organ movement.
- **Rotary speaker:** SLOW / FAST operation, fine speed adjustment and ROTOR MIX.

### 03 / Play, split and harmonize

Use the FM-1 keys or MIDI input to play B1. Its performance controls include:

- **Four keyboard ranges** for routing the different organ parts and chord behavior.
- **Chord assignments** and **CHORD ATTACK**, including humanized note-on timing.
- **48 scales**, with root-note, **KEYLOCK** and **GUIDE** options.
- **Global octave** and MIDI/range routing.

These settings are part of the project, so a registration can include more than just the drawbar positions.

### 04 / Navigate and edit

B1 uses the FM-1's screen, rotary controls and buttons. Open a parameter page, then use its labelled knobs to change the values shown on screen.

For the effects pages, **K1–K4** correspond to the parameters listed below. In the project loader, turn **K1** or **SELECT** to choose a slot and press the physical **SEL** button (second button from the left in the upper row) to load it.

## Effects

B1 combines the organ's built-in modulation with separate delay and reverb pages.

### Rotary speaker and scanner

The rotary effect provides slow/fast motion, fine speed control and a blend setting. Scanner chorus/vibrato adds a separate modulation character. Both can be used to give sustained notes movement.

### FX 5/6 · Delay

| Control | Parameter | Range |
| :-- | :-- | :-- |
| K1 | MIX | 0–100 |
| K2 | TIME | 0–100 (approximately 32–350 ms) |
| K3 | FEEDBACK | 0–90 |
| K4 | STEREO | 0–100 |

### FX 6/6 · Reverb

| Control | Parameter | Choices / range |
| :-- | :-- | :-- |
| K1 | TYPE | OFF, SPRING, ROOM, HALL, CATHEDRAL |
| K2 | MIX | 0–100 |
| K3 | DECAY | 0–100 |
| K4 | DAMP | 0–100 |

Delay and reverb can continue to ring out briefly after you release the keys. With the effects off, a dry registration returns to silence.

## Projects

B1 stores **12 named projects** in persistent memory. A project holds the complete organ setup, including:

- All nine drawbars for UPPER, LOWER and SOLO.
- Keyboard ranges, chord assignments and chord attack.
- Scale, root, KEYLOCK, GUIDE and global octave.
- Percussion, key click, leakage, amplifier and organ bank.
- Rotary and scanner settings, delay and reverb.

To **load** a project, open the project loader, choose a slot with **K1** or **SELECT**, then press **SEL**. A successful operation shows `PROJECT LOADED` on screen. Saving uses the project's save function and confirms with `PROJECT SAVED`.

Existing saved projects keep their stored settings. New or reset projects start with B1's factory settings.

## Installation

The firmware is intended for the **M-VAVE FM-1**. The repository contains both an update package and a browser-based installer.

| File / directory | Purpose |
| :-- | :-- |
| `B1-WebInstaller.html` | WebMIDI-based installer for a supported browser and compatible device |
| `build/SikkenWorks-B1-V2.6-BEETLE-OTA.fwsc` | Firmware update package |
| `build/BEETLE-RC1.01-RESTORE-ORIGINAL.fwsc` | Original BEETLE reference package, included for recovery planning |
| `firmware/` | Full C firmware source and platform integration |
| `tools/` and `tests/` | Build and verification utilities |
| `ui-captures/` | Screen captures and UI assets |
| `audio-reference/` | Software-rendered audio examples |

**Before installing:** back up your projects and keep an independent, tested recovery method. The firmware package has undergone offline checks, but a successful flash, boot and rollback on a physical FM-1 are **not guaranteed**. In particular, the provided BEETLE reference should not be treated as a universal recovery image for every firmware or hardware revision.

## Credits

**B1** is developed by **SikkenWorks**.

B1 incorporates platform work from **Felucca**, developed by **Leo Kuroshita / Hügelton Instruments** and contributors. The applicable hardware abstraction, USB/OTA integration and associated files retain their original copyright notices.

B1 is released under **GPL-3.0-only**. The included Inter font is subject to its own **SIL Open Font License 1.1** (`INTER-OFL-1.1.txt`). See [`CREDITS.md`](CREDITS.md) and the accompanying license texts for the complete attribution and terms.

---

<div align="center">

**SikkenWorks** · Born out of passion.

[Version history](CHANGELOG.md)

</div>
