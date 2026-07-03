#pragma once
#include <stdint.h>
#include "fb.h"

/* Clock position cycle (A button steps through these in order) */
typedef enum {
    CLOCK_BOTTOM_RIGHT = 0,
    CLOCK_BOTTOM_LEFT,
    CLOCK_TOP_RIGHT,
    CLOCK_TOP_LEFT,
    CLOCK_CENTER,       /* larger font, both axes centred */
    CLOCK_OFF,
    CLOCK_POS_COUNT
} ClockPos;

typedef struct {
    ClockPos clock_pos;
    int      date_visible;
    int      hints_visible;
    double   hints_hide_at;
    char     clock_str[8];
    char     date_str[12];
    double   last_update;
    /* now-playing banner + cover */
    char     np_composer[64];
    char     np_title[96];
    double   np_hide_at;
    uint8_t *cover_px;      /* RGBA, malloc'd — NULL if none */
    int      cover_src_w;
    int      cover_src_h;
    /* exit confirmation dialog */
    int      confirm_exit;
} OSDState;

void osd_init(OSDState *o, double now);
void osd_update(OSDState *o, double now);
void osd_draw(const OSDState *o, FBDev *fb);
void osd_draw_confirm(const OSDState *o, FBDev *fb);
/* composer and title shown as two separate lines; cover_px ownership is taken */
void osd_show_track(OSDState *o, double now,
                    const char *composer, const char *title,
                    uint8_t *cover_px, int cover_w, int cover_h);
void osd_free_cover(OSDState *o);
