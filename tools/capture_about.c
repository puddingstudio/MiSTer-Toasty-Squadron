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
 * Build & run (from repo root):
 *   zig cc -O2 -Isrc -DAPP_VERSION=\"dev\" -o tools/capture_about \
 *       tools/capture_about.c src/fb.c src/about.c src/sprite.c -lm -lpthread
 *   ./tools/capture_about 60
 *   python3 tools/capture_about_gif.py /tmp 640 288 docs/about.gif
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "fb.h"
#include "about.h"

/* sprite.c (linked in only for stb_image's implementation, which about.c's
 * own #include "stb_image.h" needs — see about.c's comment) references
 * these two extern globals normally defined in main.c. */
int   g_screen_h = 288;
float g_pixel_aspect_r = 0.60f;   /* PAL: (4*288)/(3*640) */

int main(int argc, char **argv)
{
    int frame_count = argc > 1 ? atoi(argv[1]) : 60;

    FBDev fb = {0};
    fb.width = 640; fb.height = 288; fb.stride = fb.width * 4;
    fb.mem  = calloc(1, (size_t)fb.stride * fb.height);
    fb.back = calloc(1, (size_t)fb.stride * fb.height);
    if (!fb.mem || !fb.back) { fprintf(stderr, "alloc failed\n"); return 1; }

    about_init("assets");

    for (int i = 0; i < frame_count; i++) {
        about_draw(&fb, 0);
        fb_flip(&fb);   /* about_draw only touches fb->back; this app's own
                          * main loop calls fb_flip separately after it. */
        char path[64];
        snprintf(path, sizeof(path), "/tmp/about_frame_%03d.raw", i);
        FILE *f = fopen(path, "wb");
        if (f) {
            fwrite(fb.mem, 1, (size_t)fb.stride * fb.height, f);
            fclose(f);
        }
        usleep(40000);
    }
    printf("%d %d %d\n", fb.width, fb.height, fb.stride);
    return 0;
}
