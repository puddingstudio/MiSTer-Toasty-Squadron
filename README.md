# Toasty Squadron

Our reimagined take on the classic Flying Toasters. [Toasty Squadron](http://toastysquadron.com) is our ongoing project — and this is us downscaling it to run natively on CRT TVs through MiSTer FPGA. Animated sprites drift across the screen while classical music plays. Includes a clock display, now-playing banner with cover art, and gamepad controls.

> Designed and tested exclusively on a CRT TV. All visuals and proportions are tuned for CRT output via MiSTer's direct video. Results on other displays may vary.

## Installation

Download the latest release zip from the [Releases](../../releases) page and unzip it. Copy the contents to your MiSTer:

```
/media/fat/toasty/
├── toasty-squadron-arm
├── assets/
├── covers/
└── music/              ← your MP3 files (not included)
```

Copy the launch script to the MiSTer Scripts folder:

```
/media/fat/Scripts/ToastySquadron.sh
```

Then select **Scripts → ToastySquadron** from the MiSTer menu.

## Controls

| Button | Action |
|---|---|
| B | Exit (with confirm) |
| A | Cycle clock position |
| Y | Toggle date display |
| D-pad Up / Down | Toggle music play / stop |
| D-pad Left | Previous track |
| D-pad Right | Next track |
| Start | About screen |

## Music

Add any MP3 or OGG files to the `music/` folder and the app will play them all. The following tracks get full cover art and metadata automatically:

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
make arm
```

Cross-compiles for `arm-linux-gnueabihf` targeting glibc 2.31 (MiSTer's DE10-Nano).

---

*made over the weekends at pudding*  
https://pudding.studio
