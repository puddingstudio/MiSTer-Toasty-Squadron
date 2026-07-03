#!/bin/bash
CONFIG="/media/fat/toasty/screensaver.cfg"
SCREENSAVER="/media/fat/Scripts/ToastySquadron.sh"
PIDFILE="/tmp/toasty_ss.pid"

# Single-instance guard
if [ -f "$PIDFILE" ]; then
    OLD=$(cat "$PIDFILE" 2>/dev/null)
    if kill -0 "$OLD" 2>/dev/null; then exit 0; fi
fi
echo $$ > "$PIDFILE"

ORIG_VT=$(fgconsole 2>/dev/null || echo 2)
SS_PID=""

cleanup() {
    [ -n "$SS_PID" ] && kill "$SS_PID" 2>/dev/null
    chvt "$ORIG_VT" 2>/dev/null
    rm -f "$PIDFILE"
    exit 0
}
trap cleanup TERM INT

while true; do
    IDLE_TIMEOUT=$(cat "$CONFIG" 2>/dev/null | tr -d '[:space:]')
    expr "$IDLE_TIMEOUT" + 0 >/dev/null 2>&1 || IDLE_TIMEOUT=180

    INPUT_DEV=""
    for dev in /dev/input/event*; do [ -e "$dev" ] && { INPUT_DEV="$dev"; break; }; done
    if [ -z "$INPUT_DEV" ]; then sleep 60; continue; fi

    if timeout "$IDLE_TIMEOUT" dd if="$INPUT_DEV" of=/dev/null bs=16 count=1 2>/dev/null; then
        sleep 1
    else
        # Switch to VT1 — fb0 becomes the active display source
        chvt 1 2>/dev/null

        "$SCREENSAVER" &
        SS_PID=$!

        while kill -0 $SS_PID 2>/dev/null; do sleep 2; done
        SS_PID=""

        # Restore original VT — MiSTer menu visible again
        chvt "$ORIG_VT" 2>/dev/null
        sleep 1
    fi
done
