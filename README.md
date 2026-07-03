# Toasty Squadron

A framebuffer screensaver for [MiSTer FPGA](https://github.com/MiSTer-devel/Main_MiSTer/wiki). Animated pixel-art sprites drift across the screen while classical music plays in the background. Includes a clock display, now-playing banner with cover art, and gamepad controls.

## Requirements

- MiSTer FPGA (DE10-Nano)
- 8BitDo or similar USB gamepad (SNES layout recommended)
- Music files in MP3 format (see below)

## Installation

Copy the release files to `/media/fat/toasty/` on your MiSTer:

```
/media/fat/toasty/
├── toasty-squadron-arm   ← compiled binary
├── assets/               ← sprite sheets and images
├── covers/               ← album artwork
└── music/                ← your MP3 files (not included)
```

Then launch it — either SSH in or add it to your MiSTer menu:

```sh
cd /media/fat/toasty
TOASTY_ASSETS=assets TOASTY_MUSIC=music TOASTY_COVERS=covers ./toasty-squadron-arm
```

### Autostart via daemon script

```sh
./screensaver-daemon.sh
```

The daemon monitors for MiSTer menu activity and launches/kills the screensaver automatically.

## Controls

| Button | Action |
|---|---|
| B | Exit (with confirm) |
| A | Cycle clock position |
| Y | Toggle date display |
| D-pad Up | Play / resume music |
| D-pad Down | Stop music |
| D-pad Left | Previous track |
| D-pad Right | Next track |
| Start | About screen |

## Music

Toasty Squadron ships without audio files — add your own MP3s to the `music/` folder. The following tracks are officially supported with cover art and metadata:

| Cover | Composer | Title |
|---|---|---|
| ![J.S. Bach](covers/cover-bach.png) | J.S. Bach | Air on the G String |
| ![Beethoven](covers/cover-beethoven.png) | Beethoven | Sonata No.15 — Pastoral Andante |
| ![Chopin](covers/cover-chopin.png) | Chopin | Nocturne Op.9 No.2 |
| ![Chopin](covers/cover-chopin.png) | Chopin | Waltz |
| ![Schumann](covers/cover-schumann.png) | Schumann | Kinderszenen Op.15 No.1 |

Name your MP3 files so the filename starts with the key — e.g. `bach.mp3`, `chopin-noc.mp3`, `chopin-wal.mp3`, `schumann.mp3`.

## Building from Source

Requires [Zig](https://ziglang.org/) for cross-compilation:

```sh
make
```

The Makefile cross-compiles for `arm-linux-gnueabihf` targeting glibc 2.31 (MiSTer's DE10-Nano).

---

![](assets/nead.png)

*made over the weekends at pudding*  
https://pudding.studio
