#pragma once
#include <stdint.h>
#include "fb.h"

typedef struct {
    int      visible;
    int      selected;         /* 0 = screensaver toggle, 1 = timeout */

    /* Animated mascot */
    uint8_t *gif_pixels;
    int      gif_w;
    int      gif_h;
    int      gif_frames;
    int     *gif_delays;
    int      gif_frame;
    float    gif_timer;

    int      screensaver_on;
    int      timeout_idx;      /* index into timeout_options[] */
    int      request_quit;     /* set by daemon_disable — caller should exit */
} MenuState;

void menu_init(MenuState *m, const char *assets_dir);
void menu_free(MenuState *m);
void menu_update(MenuState *m, float dt);
void menu_draw(const MenuState *m, FBDev *fb);
void menu_nav(MenuState *m, int dir);       /* up/down: -1/+1 */
void menu_adjust(MenuState *m, int dir);    /* left/right on timeout: -1/+1 */
void menu_confirm(MenuState *m);
