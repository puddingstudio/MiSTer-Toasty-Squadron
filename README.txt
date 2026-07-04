MiSTer Toasty Squadron v1.0
===========================

Our reimagined take on the classic Flying Toasters, Toasty Squadron brings
the iconic screensaver to CRT TVs through MiSTer FPGA. Animated sprites drift
across the screen while classical music plays, creating an authentic retro
experience. Features include a real-time clock, a now-playing banner with
cover art, and full gamepad support.

Designed and tested exclusively on a CRT TV. All visuals and proportions are
tuned for CRT output via MiSTer's direct video.


INSTALLATION
------------

Copy the contents of this zip to your MiSTer:

  /media/fat/toasty/
  ├── toasty-squadron-arm
  ├── assets/
  ├── covers/
  └── music/

Copy the launch script to the MiSTer Scripts folder:

  /media/fat/Scripts/ToastySquadron.sh

Then select Scripts → ToastySquadron from the MiSTer menu.


CONTROLS
--------

  B               Exit (with confirm)
  A               Cycle clock position
  Y               Toggle date display
  D-pad Up/Down   Toggle music play / stop
  D-pad Left      Previous track
  D-pad Right     Next track
  Start           About screen


MUSIC
-----

The following classical recordings are included:

  J.S. Bach     — Air on the G String (BWV 1068)
  Beethoven     — Piano Sonata No.15 "Pastoral", Op.28 – II. Andante
  Chopin        — Nocturne in E-flat Major, Op.9, No.2
  Chopin        — Waltz in A Minor, B.150
  Schumann      — Kinderszenen Op.15, No.1

All recordings provided by Musopen (https://musopen.org).
Public domain — free to use for any purpose.

You can also add your own MP3 or OGG files to the music/ folder.


MORE INFO
---------

https://toastysquadron.com
https://pudding.studio

CHANGELOG
---------

v1.0
  - Auto-update: About screen checks for new releases and installs
    with one button press
  - Added waffle sprite to screensaver rotation
  - Prefixed title with MiSTer in README

v0.9
  - Initial release
  - Animated toasters with a variety of breakfast-themed sprites
  - Classical music playback with now-playing banner and cover art
  - Real-time clock with toggleable date display
  - Gamepad support
  - About screen with version number


Made over the weekends at Pudding.
