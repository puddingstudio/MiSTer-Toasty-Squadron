#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "osd.h"
#include "fb.h"
#include "font8x8.h"
#include "config.h"

#define HINTS_TIMEOUT   8.0
#define OSD_PAD         3
#define SAFE_X          32
#define SAFE_Y          20
#define NP_TIMEOUT      3.5

/* Cover art display size on screen (PAR-corrected) */
#define COVER_W         64
#define COVER_H         ((int)(COVER_W * g_pixel_aspect_r))

static const char HINT_TEXT[]    = "A:clock  Y:date  B:exit  dpad:music";
static const char CONFIRM_TEXT[] = "Exit? [A: yes   B: no]";

/* ── font rendering ─────────────────────────────────────────────────────── */

static void draw_char(FBDev *fb, int x, int y, unsigned char c, int scale,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    if (c >= 128) c = '?';
    const uint8_t *glyph = font8x8_basic[c];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            int lit = (bits >> col) & 1;
            if (lit)
                fb_fill_rect_alpha(fb, x + col * scale, y + row * scale,
                                   scale, scale, r, g, b, alpha);
        }
    }
}

static void draw_text(FBDev *fb, int x, int y, const char *str, int scale,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t fg_alpha)
{
    for (int i = 0; str[i]; i++)
        draw_char(fb, x + i * 8 * scale, y, (unsigned char)str[i],
                  scale, r, g, b, fg_alpha);
}

static void draw_text_bg(FBDev *fb, int x, int y, const char *str, int scale,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t fg_alpha)
{
    int len = (int)strlen(str);
    int tw  = len * 8 * scale;
    int th  = 8 * scale;
    fb_fill_rect_alpha(fb, x - OSD_PAD, y - OSD_PAD,
                       tw + 2 * OSD_PAD, th + 2 * OSD_PAD, 0, 0, 0, 170);
    draw_text(fb, x, y, str, scale, r, g, b, fg_alpha);
}

/* ── cover art ──────────────────────────────────────────────────────────── */

static void draw_cover(FBDev *fb, const OSDState *o, int x, int y)
{
    if (!o->cover_px) return;
    /* nearest-neighbour scale from cover_src_w×cover_src_h → COVER_W×COVER_H */
    for (int dy = 0; dy < COVER_H; dy++) {
        int sy = dy * o->cover_src_h / COVER_H;
        for (int dx = 0; dx < COVER_W; dx++) {
            int sx = dx * o->cover_src_w / COVER_W;
            const uint8_t *p = o->cover_px + (sy * o->cover_src_w + sx) * 4;
            uint8_t a = p[3];
            if (a == 0) continue;
            int px = x + dx, py = y + dy;
            if (px < 0 || px >= fb->width || py < 0 || py >= fb->height) continue;
            uint32_t *dst = (uint32_t *)(fb->back + py * fb->stride) + px;
            uint32_t  bg  = *dst;
            uint32_t or_ = (p[0] * a + ((bg >> 16) & 0xFF) * (255 - a)) >> 8;
            uint32_t og  = (p[1] * a + ((bg >>  8) & 0xFF) * (255 - a)) >> 8;
            uint32_t ob  = (p[2] * a + ( bg        & 0xFF) * (255 - a)) >> 8;
            *dst = (or_ << 16) | (og << 8) | ob;
        }
    }
}

/* ── public API ─────────────────────────────────────────────────────────── */

void osd_free_cover(OSDState *o)
{
    if (o->cover_px) { free(o->cover_px); o->cover_px = NULL; }
    o->cover_src_w = o->cover_src_h = 0;
}

void osd_show_track(OSDState *o, double now,
                    const char *composer, const char *title,
                    uint8_t *cover_px, int cover_w, int cover_h)
{
    osd_free_cover(o);
    snprintf(o->np_composer, sizeof(o->np_composer), "%s", composer ? composer : "");
    snprintf(o->np_title,    sizeof(o->np_title),    "%s", title    ? title    : "(stopped)");
    o->np_hide_at  = now + NP_TIMEOUT;
    o->cover_px    = cover_px;
    o->cover_src_w = cover_w;
    o->cover_src_h = cover_h;
}

void osd_init(OSDState *o, double now)
{
    memset(o, 0, sizeof(*o));
    o->clock_pos     = CLOCK_CENTER;
    o->hints_visible = 1;
    o->hints_hide_at = now + HINTS_TIMEOUT;
    o->last_update   = -1.0;
    osd_update(o, now);
}

void osd_update(OSDState *o, double now)
{
    if (o->hints_visible && now >= o->hints_hide_at)
        o->hints_visible = 0;

    if (o->np_hide_at > 0 && now >= o->np_hide_at) {
        o->np_composer[0] = o->np_title[0] = '\0';
        o->np_hide_at = 0;
        osd_free_cover(o);
    }

    if (now - o->last_update < 1.0) return;
    o->last_update = now;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    snprintf(o->clock_str, sizeof(o->clock_str),
             "%02d:%02d", tm->tm_hour, tm->tm_min);
    snprintf(o->date_str, sizeof(o->date_str),
             "%02d/%02d/%04d", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
}

void osd_draw(const OSDState *o, FBDev *fb)
{
    /* ── exit confirmation ── */
    if (o->confirm_exit) {
        int scale = 2;
        int tw    = (int)strlen(CONFIRM_TEXT) * 8 * scale;
        int th    = 8 * scale;
        int cx    = (SCREEN_W - tw) / 2;
        int cy    = (SCREEN_H - th) / 2;
        fb_fill_rect_alpha(fb, cx - 12, cy - 12, tw + 24, th + 24, 0, 0, 0, 210);
        draw_text(fb, cx, cy, CONFIRM_TEXT, scale, 255, 220, 80, 240);
        return;  /* don't draw anything else while confirming */
    }

    /* ── clock ── */
    if (o->clock_pos != CLOCK_OFF) {
        int scale = (o->clock_pos == CLOCK_CENTER) ? 4 : 2;
        int tw    = (int)strlen(o->clock_str) * 8 * scale;
        int th    = 8 * scale;
        int cx, cy;

        switch (o->clock_pos) {
        case CLOCK_BOTTOM_RIGHT: cx = SCREEN_W - tw - SAFE_X; cy = SCREEN_H - th - SAFE_Y; break;
        case CLOCK_BOTTOM_LEFT:  cx = SAFE_X;                 cy = SCREEN_H - th - SAFE_Y; break;
        case CLOCK_TOP_RIGHT:    cx = SCREEN_W - tw - SAFE_X; cy = SAFE_Y;                 break;
        case CLOCK_TOP_LEFT:     cx = SAFE_X;                 cy = SAFE_Y;                 break;
        case CLOCK_CENTER:       cx = (SCREEN_W - tw) / 2;  cy = (SCREEN_H - th) / 2;  break;
        default:                 cx = cy = 0; break;
        }

        draw_text(fb, cx, cy, o->clock_str, scale, 255, 255, 255, 220);

        if (o->date_visible) {
            int ds  = 1;
            int dtw = (int)strlen(o->date_str) * 8 * ds;
            int dth = 8 * ds;
            int dx  = cx + (tw - dtw) / 2;
            int dy  = (o->clock_pos == CLOCK_BOTTOM_RIGHT ||
                       o->clock_pos == CLOCK_BOTTOM_LEFT)
                      ? cy - dth - 5
                      : cy + th + 5;
            draw_text(fb, dx, dy, o->date_str, ds, 220, 220, 180, 200);
        }
    }

    /* ── now-playing banner + cover ── */
    if (o->np_hide_at > 0 && o->np_title[0]) {
        if (o->cover_px)
            draw_cover(fb, o, SAFE_X, SAFE_Y);

        int text_x = o->cover_px ? (SAFE_X + COVER_W + 6) : SAFE_X;
        /* Composer on top row (yellow), title below (white) */
        if (o->np_composer[0])
            draw_text_bg(fb, text_x, SAFE_Y,      o->np_composer, 1, 255, 220,  80, 230);
        draw_text_bg(    fb, text_x, SAFE_Y + 12, o->np_title,    1, 220, 220, 220, 220);
    }

    /* ── hint bar ── */
    if (o->hints_visible) {
        int tw = (int)strlen(HINT_TEXT) * 8;
        int x  = (SCREEN_W - tw) / 2;
        int y  = SCREEN_H - 8 - SAFE_Y;
        draw_text_bg(fb, x, y, HINT_TEXT, 1, 200, 200, 200, 200);
    }
}

void osd_draw_confirm(const OSDState *o, FBDev *fb)
{
    if (!o->confirm_exit) return;
    int scale = 2;
    int tw    = (int)strlen(CONFIRM_TEXT) * 8 * scale;
    int th    = 8 * scale;
    int cx    = (SCREEN_W - tw) / 2;
    int cy    = (SCREEN_H - th) / 2;
    fb_fill_rect_alpha(fb, cx - 12, cy - 12, tw + 24, th + 24, 0, 0, 0, 210);
    draw_text(fb, cx, cy, CONFIRM_TEXT, scale, 255, 220, 80, 240);
}
