#!/bin/bash
printf "\033[?25l"
if [ -f /tmp/toasty_apply_update.sh ]; then
    bash /tmp/toasty_apply_update.sh
    rm -f /tmp/toasty_apply_update.sh
fi
cd /media/fat/toasty
env TOASTY_ASSETS=assets TOASTY_MUSIC=music TOASTY_COVERS=covers ./toasty-squadron-arm >/dev/null 2>&1
printf "\033[?25h"
