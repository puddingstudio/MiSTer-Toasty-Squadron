# MiSTer Toasty Squadron

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

<table width="100%"><thead><tr><th>Cover</th><th>Composer</th><th>Work</th><th>Source</th></tr></thead><tbody>
<tr><td><img src="covers/cover-bach.png"></td><td>J.S. Bach</td><td>Air on the G String (BWV 1068)</td><td><a href="https://musopen.org/music/3775-orchestral-suite-no-3-in-d-major-bwv-1068/">Musopen</a></td></tr>
<tr><td><img src="covers/cover-beethoven.png"></td><td>Beethoven</td><td>Piano Sonata No.15 "Pastoral", Op.28 – II. Andante</td><td><a href="https://musopen.org/music/2567-piano-sonata-no-15-in-d-major-op-28-pastoral/">Musopen</a></td></tr>
<tr><td><img src="covers/cover-chopin.png"></td><td>Chopin</td><td>Nocturne in E♭ Major, Op.9, No.2</td><td><a href="https://musopen.org/music/108-nocturnes-op-9/">Musopen</a></td></tr>
<tr><td><img src="covers/cover-chopin.png"></td><td>Chopin</td><td>Waltz in A Minor, B.150</td><td><a href="https://musopen.org/music/4406-waltz-in-a-minor-b-150/">Musopen</a></td></tr>
<tr><td><img src="covers/cover-schumann.png"></td><td>Schumann</td><td>Kinderszenen Op.15, No.1</td><td><a href="https://musopen.org/music/2326-scenes-from-childhood-op-15/">Musopen</a></td></tr>
</tbody></table>

<a href="https://musopen.org"><img src=".github/images/musopen-logo.png" width="170"></a>

All recordings are provided by **[Musopen](https://musopen.org)** — a non-profit dedicated to free, unrestricted access to classical music. All tracks are **public domain** and may be used freely for any purpose, commercial or non-commercial.

You can also add your own MP3 or OGG files to the `music/` folder and the app will include them in the rotation.

## Building from Source

Requires [Zig](https://ziglang.org/) for cross-compilation:

```sh
make arm
```

Cross-compiles for `arm-linux-gnueabihf` targeting glibc 2.31 (MiSTer's DE10-Nano).

## Changelog

### v1.0 (rebuilt)
- Fix: rebuilt release — the initial v1.0 build was missing the waffle sprite asset (asset15), which caused the app to crash on launch. That asset is now bundled correctly
- Auto-update: About screen checks for new releases and installs with one button press
- Added waffle sprite to screensaver rotation
- Prefixed title with MiSTer in README

### v0.9
- Initial release
- Animated toasters with a variety of breakfast-themed sprites
- Classical music playback with now-playing banner and cover art
- Real-time clock with toggleable date display
- Gamepad support
- About screen with version number

---

| <a href="https://pudding.studio"><img src=".github/images/pudding.gif" width="100"></a> | *made over the weekends at pudding*<br>https://pudding.studio | <a href="https://grujicic.com"><img src=".github/images/nenad.png" width="100"></a> |
|:---:|:---:|:---:|
