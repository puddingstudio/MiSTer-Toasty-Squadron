# Toasty Squadron

Our reimagined take on the classic Flying Toasters, **[Toasty Squadron](http://toastysquadron.com)** is an ongoing project that brings the iconic screensaver to CRT TVs through MiSTer FPGA. Animated sprites drift across the screen while classical music plays, creating an authentic retro experience. Features include a real-time clock, a now-playing banner with cover art, and full gamepad support.

> Designed and tested exclusively on a CRT TV. All visuals and proportions are tuned for CRT output via MiSTer's direct video. Results on other displays may vary.

## Installation

Download the latest release zip from the [Releases](../../releases) page and unzip it. Copy the contents to your MiSTer:

```
/media/fat/toasty/
├── toasty-squadron-arm
├── assets/
├── covers/
└── music/
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

The following classical recordings are included and play automatically with full cover art and metadata:

| Cover | Composer | Work | Source |
|---|---|---|---|
| ![J.S. Bach](covers/cover-bach.png) | J.S. Bach | Air on the G String (BWV 1068) | [Musopen](https://musopen.org/music/3775-orchestral-suite-no-3-in-d-major-bwv-1068/) |
| ![Beethoven](covers/cover-beethoven.png) | Beethoven | Piano Sonata No.15 "Pastoral", Op.28 – II. Andante | [Musopen](https://musopen.org/music/2567-piano-sonata-no-15-in-d-major-op-28-pastoral/) |
| ![Chopin](covers/cover-chopin.png) | Chopin | Nocturne in E♭ Major, Op.9, No.2 | [Musopen](https://musopen.org/music/108-nocturnes-op-9/) |
| ![Chopin](covers/cover-chopin.png) | Chopin | Waltz in A Minor, B.150 | [Musopen](https://musopen.org/music/4406-waltz-in-a-minor-b-150/) |
| ![Schumann](covers/cover-schumann.png) | Schumann | Kinderszenen Op.15, No.1 | [Musopen](https://musopen.org/music/2326-scenes-from-childhood-op-15/) |

All recordings are provided by **[Musopen](https://musopen.org)** — a non-profit dedicated to free, unrestricted access to classical music. All tracks are **public domain** and may be used freely for any purpose, commercial or non-commercial.

[![Musopen](.github/images/musopen-logo.svg)](https://musopen.org)

You can also add your own MP3 or OGG files to the `music/` folder and the app will include them in the rotation.

## Building from Source

Requires [Zig](https://ziglang.org/) for cross-compilation:

```sh
make arm
```

Cross-compiles for `arm-linux-gnueabihf` targeting glibc 2.31 (MiSTer's DE10-Nano).

## Changelog

### v0.9
- Initial release
- Animated toaster and toast sprites
- Classical music playback with now-playing banner and cover art
- Real-time clock with toggleable date display
- Gamepad support
- About screen with version number

---

*made over the weekends at pudding*  
https://pudding.studio
