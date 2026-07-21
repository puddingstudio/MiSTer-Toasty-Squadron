/*
 * Hidden dev tool, not part of the shipped app UI: renders the About
 * screen's starfield + waffle animation (minus the version/update footer)
 * to a sequence of raw BGRX framebuffer dumps under /tmp, for
 * tools/capture_about_gif.py to turn into a marketing GIF for the repo's
 * README (see MiSTerFin's identical tool/workflow, which this mirrors).
 *
 * Doesn't touch the real app's main.c/main() at all — fakes an FBDev
 * backed by plain malloc'd buffers (fb_open needs a real fbdev ioctl,
 * which a desktop build doesn't have) and calls about_init/about_draw
 * directly.
 *
 * about_draw()'s waffle animation advances one sprite frame every
 * WAFFLE_TICK_ADVANCE calls (about.c's own g_waffle_tick counter) — the
 * real app calls about_draw() once per main-loop tick at TARGET_FPS, so
 * each waffle pose is actually held for WAFFLE_TICK_ADVANCE/TARGET_FPS
 * seconds. First version of this tool called about_draw() once per
 * *captured* frame at a fixed 40ms spacing, which (a) held each pose 40ms
 * * WAFFLE_TICK_ADVANCE instead of the real ~83ms — several times slower
 * than on a real screen — and (b) with only 60 captured frames, never
 * advanced past pose 60/WAFFLE_TICK_ADVANCE = 12 of 42, so the loop never
 * completed a full rotation (confirmed by the user: "looks slow and cuts
 * off"). Ticking through every real frame and only saving one dump per
 * pose change fixes both: exactly WAFFLE_FRAMES output frames, one full
 * rotation, each meant to be held for the real duration (set
 * tools/capture_about_gif.py's DELAY_CS to match — see that script).
 *
 * Build & run (from repo root):
 *   zig cc -O2 -Isrc -DAPP_VERSION=\"dev\" -o tools/capture_about \
 *       tools/capture_about.c src/fb.c src/about.c src/sprite.c -lm -lpthread
 *   ./tools/capture_about
 *   python3 tools/capture_about_gif.py /tmp 640 288 docs/about.gif
 */
#include <stdio.h>
#include <stdlib.h>
#include "fb.h"
#include "about.h"
#include "config.h"

/* sprite.c (linked in only for stb_image's implementation, which about.c's
 * own #include "stb_image.h" needs — see about.c's comment) references
 * these two extern globals normally defined in main.c. */
int   g_screen_h = 288;
float g_pixel_aspect_r = 0.60f;   /* PAL: (4*288)/(3*640) */

#define WAFFLE_TICK_ADVANCE 5   /* matches about.c's own g_waffle_tick threshold */
#define WAFFLE_FRAMES (SPEC_FRAMES[NUM_SPECS - 1])   /* about.c's WAFFLE_SPEC_INDEX */

int main(void)
{
    FBDev fb = {0};
    fb.width = 640; fb.height = 288; fb.stride = fb.width * 4;
    fb.mem  = calloc(1, (size_t)fb.stride * fb.height);
    fb.back = calloc(1, (size_t)fb.stride * fb.height);
    if (!fb.mem || !fb.back) { fprintf(stderr, "alloc failed\n"); return 1; }

    about_init("assets");

    int total_ticks = WAFFLE_FRAMES * WAFFLE_TICK_ADVANCE;
    int saved = 0;
    for (int i = 0; i < total_ticks; i++) {
        about_draw(&fb, 0);
        fb_flip(&fb);   /* about_draw only touches fb->back; this app's own
                          * main loop calls fb_flip separately after it. */
        if (i % WAFFLE_TICK_ADVANCE != 0) continue;   /* one dump per held pose, not per tick */
        char path[64];
        snprintf(path, sizeof(path), "/tmp/about_frame_%03d.raw", saved++);
        FILE *f = fopen(path, "wb");
        if (f) {
            fwrite(fb.mem, 1, (size_t)fb.stride * fb.height, f);
            fclose(f);
        }
    }
    printf("%d %d %d\n", fb.width, fb.height, fb.stride);
    return 0;
}
