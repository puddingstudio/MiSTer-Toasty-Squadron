SRCS = src/main.c src/sprite.c src/anim.c src/render.c src/fb.c src/osd.c src/music.c src/id3cover.c src/about.c

TARGET     = toasty-squadron
TARGET_ARM = toasty-squadron-arm

VERSION ?= $(shell git describe --tags --abbrev=0 2>/dev/null || echo "dev")

# ── Native build ─────────────────────────────────────────────────────────────
# NOTE: fb.c uses <linux/fb.h> which only exists on Linux.
# Native build works on Linux only. On macOS, use 'make arm' instead.
CC     = gcc
CFLAGS = -O2 -Wall -Wextra -Isrc -DAPP_VERSION=\"$(VERSION)\"
LIBS   = -lm

# ── ARM cross-compile for MiSTer (Cortex-A9, glibc 2.31) ───────────────────
# Requires: brew install zig
# No external libraries needed — pure libc + libm.
CC_ARM     = zig cc -target arm-linux-gnueabihf.2.31 -mcpu=cortex_a9
CFLAGS_ARM = -O2 -Isrc -DAPP_VERSION=\"$(VERSION)\"
LIBS_ARM   = -lm

.PHONY: all arm clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

arm: $(SRCS)
	$(CC_ARM) $(CFLAGS_ARM) -o $(TARGET_ARM) $^ $(LIBS_ARM)

clean:
	rm -f $(TARGET) $(TARGET_ARM)
