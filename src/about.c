#include "about.h"
#include "fb.h"
#include "config.h"
#include "stb_image.h"
#include "font8x8.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void draw_char(FBDev *fb, int x, int y, unsigned char c, int scale,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    if (c >= 128) c = '?';
    const uint8_t *glyph = font8x8_basic[c];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if ((bits >> col) & 1)
                fb_fill_rect_alpha(fb, x + col * scale, y + row * scale,
                                   scale, scale, r, g, b, alpha);
        }
    }
}

static void draw_text(FBDev *fb, int x, int y, const char *str, int scale,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    for (int i = 0; str[i]; i++)
        draw_char(fb, x + i * 8 * scale, y, (unsigned char)str[i],
                  scale, r, g, b, alpha);
}

static uint8_t *g_px = NULL;
static int      g_w  = 0;
static int      g_h  = 0;

void about_init(const char *assets_dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/about.png", assets_dir);
    int ch;
    g_px = stbi_load(path, &g_w, &g_h, &ch, 4);
}

void about_free(void)
{
    if (g_px) { stbi_image_free(g_px); g_px = NULL; }
    g_w = g_h = 0;
}

void about_draw(FBDev *fb)
{
    /* Black background */
    for (int y = 0; y < fb->height; y++) {
        uint32_t *row = (uint32_t *)(fb->back + y * fb->stride);
        for (int x = 0; x < fb->width; x++)
            row[x] = 0xFFD4D4D4;  /* light grey in BGRX8888 */
    }

    if (!g_px || g_w <= 0 || g_h <= 0) return;

    static const char TITLE[] = "Toasty Squadron";
    static const char LINE1[] = "made over the weekends at pudding";
    static const char LINE2[] = "https://pudding.studio";

    const int ts    = 2;   /* title scale */
    const int s1    = 1;   /* small text scale */
    const int tch   = 8 * ts;
    const int sch   = 8 * s1;
    const int gap   = 8;   /* gap between image and title */
    const int lsp   = 4;   /* line spacing between small lines */

    /* vertically center whole block: image + gap + title + 6 + line1 + lsp + line2 */
    int total_h = g_h + gap + tch + 6 + sch + lsp + sch;
    int dx  = (fb->width - g_w) / 2;
    int dy  = (fb->height - total_h) / 2;

    fb_blit(fb, g_px, g_w, g_h, dx, dy, g_w, g_h, 255);

    int tty  = dy + g_h + gap;
    int ty1  = tty + tch + 6;
    int ty2  = ty1 + sch + lsp;
    int ttx  = (fb->width - (int)strlen(TITLE) * 8 * ts) / 2;
    int tx1  = (fb->width - (int)strlen(LINE1) * 8 * s1) / 2;
    int tx2  = (fb->width - (int)strlen(LINE2) * 8 * s1) / 2;

    draw_text(fb, ttx, tty, TITLE, ts,  40,  40,  40, 230);
    draw_text(fb, tx1, ty1, LINE1, s1,  80,  80,  80, 200);
    draw_text(fb, tx2, ty2, LINE2, s1,  30,  80, 180, 210);

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif
    {
        static const char ver[] = APP_VERSION;
        int vw = (int)strlen(ver) * 8;
        draw_text(fb, fb->width - vw - 6, fb->height - 8 - 6, ver, 1, 120, 120, 120, 160);
    }
}
