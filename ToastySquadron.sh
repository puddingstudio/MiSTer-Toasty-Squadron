#!/bin/bash
cd /media/fat/toasty
exec env TOASTY_ASSETS=assets TOASTY_MUSIC=music TOASTY_COVERS=covers ./toasty-squadron-arm >/dev/null 2>&1
