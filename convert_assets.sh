#!/usr/bin/env bash
# Convert WebP sprite assets from ../web/sprites/ to PNG in assets/
# Resizes sprite frames to 128×128 and moon to 192×192.
#
# Requires: ffmpeg  (preferred)
#       or: ImageMagick (magick / convert)

set -euo pipefail

SRC_DIR="$(cd "$(dirname "$0")/../web/sprites" && pwd)"
DST_DIR="$(cd "$(dirname "$0")" && pwd)/assets"

if ! command -v ffmpeg &>/dev/null && ! command -v magick &>/dev/null && ! command -v convert &>/dev/null; then
    echo "Error: need ffmpeg or ImageMagick installed." >&2
    exit 1
fi

do_convert() {
    local src="$1" dst="$2" w="$3" h="$4"
    if command -v ffmpeg &>/dev/null; then
        ffmpeg -y -loglevel error -i "$src" -vf "scale=${w}:${h}" "$dst"
    elif command -v magick &>/dev/null; then
        magick "$src" -resize "${w}x${h}!" "$dst"
    else
        convert "$src" -resize "${w}x${h}!" "$dst"
    fi
}

echo "Source: $SRC_DIR"
echo "Destination: $DST_DIR"

# Sprite frames (128×128)
for i in $(seq 1 14); do
    dir="$DST_DIR/asset${i}"
    mkdir -p "$dir"
    src_dir="$SRC_DIR/asset${i}"
    count=0
    for webp in "$src_dir"/asset${i}_*.webp; do
        [ -f "$webp" ] || continue
        fname="$(basename "${webp%.webp}").png"
        dst="$dir/$fname"
        do_convert "$webp" "$dst" 128 128
        count=$((count + 1))
    done
    echo "  asset${i}: $count frames"
done

# Moon (192×192)
src_moon="$SRC_DIR/moon.webp"
dst_moon="$DST_DIR/moon.png"
if [ -f "$src_moon" ]; then
    do_convert "$src_moon" "$dst_moon" 192 192
    echo "  moon: done"
else
    echo "  WARNING: moon.webp not found at $src_moon" >&2
fi

echo "Done. Assets written to $DST_DIR"
