#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "fb.h"
#include "font8x8.h"
#include "config.h"
#include "stb_image.h"

#define DAEMON_SCRIPT  "/media/fat/toasty/screensaver-daemon.sh"
#define STARTUP_FILE   "/media/fat/linux/user-startup.sh"
#define DAEMON_MARKER  "screensaver-daemon"
#define CONFIG_FILE    "/media/fat/toasty/screensaver.cfg"

static const int  TIMEOUT_SECS[]   = { 60, 180, 300, 600, 900 };
static const char *TIMEOUT_LABELS[] = { "1", "3", "5", "10", "15" };
#define TIMEOUT_COUNT 5

/* ── helpers ─────────────────────────────────────────────────────────────── */

static uint8_t rev8(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

static void draw_char(FBDev *fb, int x, int y, char c, int scale,
                      uint8_t r, uint8_t g, uint8_t b)
{
    const uint8_t *glyph = font8x8_basic[(unsigned char)c & 0x7F];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (!((bits >> col) & 1)) continue;
            for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++) {
                    int px = x + col * scale + sx;
                    int py = y + row * scale + sy;
                    if (px < 0 || px >= fb->width || py < 0 || py >= fb->height) continue;
                    uint32_t *p = (uint32_t *)(fb->back + py * fb->stride) + px;
                    *p = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
                }
        }
    }
    (void)rev8;
}

static void draw_text(FBDev *fb, int x, int y, const char *s, int scale,
                      uint8_t r, uint8_t g, uint8_t b)
{
    for (; *s; s++, x += 8 * scale)
        draw_char(fb, x, y, *s, scale, r, g, b);
}

static void draw_rect(FBDev *fb, int x, int y, int w, int h,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
    fb_fill_rect_alpha(fb, x, y, w, h, r, g, b, alpha);
}

static void blit_gif_frame(FBDev *fb, const MenuState *m, int dx, int dy, int dw, int dh)
{
    if (!m->gif_pixels) return;
    const uint8_t *frame = m->gif_pixels +
        (size_t)m->gif_frame * m->gif_w * m->gif_h * 4;

    for (int fy = 0; fy < dh; fy++) {
        int sy = fy * m->gif_h / dh;
        if (sy >= m->gif_h) sy = m->gif_h - 1;
        for (int fx = 0; fx < dw; fx++) {
            int sx = fx * m->gif_w / dw;
            if (sx >= m->gif_w) sx = m->gif_w - 1;
            const uint8_t *src = frame + (sy * m->gif_w + sx) * 4;
            uint8_t a = src[3];
            if (a == 0) continue;
            int px = dx + fx, py = dy + fy;
            if (px < 0 || px >= fb->width || py < 0 || py >= fb->height) continue;
            uint32_t *dst = (uint32_t *)(fb->back + py * fb->stride) + px;
            uint32_t d = *dst;
            uint32_t ia = 255 - a;
            uint32_t or_ = (src[0] * a + ((d >> 16) & 0xFF) * ia) >> 8;
            uint32_t og  = (src[1] * a + ((d >>  8) & 0xFF) * ia) >> 8;
            uint32_t ob  = (src[2] * a + ( d        & 0xFF) * ia) >> 8;
            *dst = (or_ << 16) | (og << 8) | ob;
        }
    }
}

/* ── timeout config ──────────────────────────────────────────────────────── */

static int load_timeout_idx(void)
{
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) return 1; /* default: 3 min */
    int secs = 180;
    fscanf(f, "%d", &secs);
    fclose(f);
    for (int i = 0; i < TIMEOUT_COUNT; i++)
        if (TIMEOUT_SECS[i] == secs) return i;
    return 1;
}

static void save_timeout_idx(int idx)
{
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) return;
    fprintf(f, "%d\n", TIMEOUT_SECS[idx]);
    fclose(f);
}

/* ── screensaver daemon management ──────────────────────────────────────── */

static int daemon_enabled(void)
{
    return system("grep -q '" DAEMON_MARKER "' " STARTUP_FILE " 2>/dev/null") == 0;
}

static void daemon_enable(void)
{
    system("grep -q '" DAEMON_MARKER "' " STARTUP_FILE " 2>/dev/null || "
           "echo '[[ -e " DAEMON_SCRIPT " ]] && " DAEMON_SCRIPT " &'"
           " >> " STARTUP_FILE);
    system(DAEMON_SCRIPT " &");
}

static void daemon_disable(void)
{
    system("sed -i '/" DAEMON_MARKER "/d' " STARTUP_FILE " 2>/dev/null");
    system("pkill -f '" DAEMON_MARKER "' 2>/dev/null");
}

/* ── public API ──────────────────────────────────────────────────────────── */

void menu_init(MenuState *m, const char *assets_dir)
{
    memset(m, 0, sizeof(*m));

    /* Load animated GIF */
    char path[512];
    snprintf(path, sizeof(path), "%s/pudding.gif", assets_dir);

    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        uint8_t *buf = (uint8_t *)malloc((size_t)sz);
        if (buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz) {
            int comp;
            m->gif_pixels = stbi_load_gif_from_memory(
                buf, (int)sz, &m->gif_delays,
                &m->gif_w, &m->gif_h, &m->gif_frames, &comp, 4);
        }
        free(buf);
        fclose(f);
    }

    m->screensaver_on = daemon_enabled();
    m->timeout_idx    = load_timeout_idx();
}

void menu_free(MenuState *m)
{
    if (m->gif_pixels) { stbi_image_free(m->gif_pixels); m->gif_pixels = NULL; }
    if (m->gif_delays) { free(m->gif_delays); m->gif_delays = NULL; }
}

void menu_update(MenuState *m, float dt)
{
    if (!m->visible || !m->gif_pixels || m->gif_frames <= 0) return;

    m->gif_timer += dt * 1000.0f; /* convert to ms */
    int delay = (m->gif_delays && m->gif_delays[m->gif_frame] > 0)
                ? m->gif_delays[m->gif_frame] : 100;

    if (m->gif_timer >= (float)delay) {
        m->gif_timer -= (float)delay;
        m->gif_frame = (m->gif_frame + 1) % m->gif_frames;
    }
}

void menu_nav(MenuState *m, int dir)
{
    int items = m->screensaver_on ? 2 : 1;
    m->selected = (m->selected + dir + items) % items;
}

void menu_adjust(MenuState *m, int dir)
{
    if (m->selected != 1 || !m->screensaver_on) return;
    m->timeout_idx = (m->timeout_idx + dir + TIMEOUT_COUNT) % TIMEOUT_COUNT;
    save_timeout_idx(m->timeout_idx);
    /* Daemon reads config at the start of each idle cycle — no restart needed */
}

void menu_confirm(MenuState *m)
{
    if (m->selected != 0) return;
    if (m->screensaver_on) {
        daemon_disable();
        m->screensaver_on = 0;
    } else {
        daemon_enable();
        m->screensaver_on = 1;
    }
}

void menu_draw(const MenuState *m, FBDev *fb)
{
    if (!m->visible) return;

    /* Dim the entire background */
    fb_fill_rect_alpha(fb, 0, 0, fb->width, fb->height, 0, 0, 0, 160);

    /* Panel dimensions — taller when timeout row is visible */
    int pw = 520, ph = m->screensaver_on ? 178 : 160;
    int px = (fb->width  - pw) / 2;
    int py = (fb->height - ph) / 2;

    /* Panel background + border */
    draw_rect(fb, px,     py,     pw,     ph,     20,  20,  35, 240);
    draw_rect(fb, px,     py,     pw,      2,    120, 100, 200, 255); /* top */
    draw_rect(fb, px,     py+ph-2,pw,      2,    120, 100, 200, 255); /* bottom */
    draw_rect(fb, px,     py,      2,     ph,    120, 100, 200, 255); /* left */
    draw_rect(fb, px+pw-2,py,      2,     ph,    120, 100, 200, 255); /* right */

    /* GIF mascot — PAR-corrected display size */
    int gif_dw = 72;
    int gif_dh = (int)(gif_dw * g_pixel_aspect_r);
    int gif_x  = px + 16;
    int gif_y  = py + (ph - gif_dh) / 2;
    blit_gif_frame(fb, m, gif_x, gif_y, gif_dw, gif_dh);

    /* Text area */
    int tx = gif_x + gif_dw + 16;
    int ty = py + 18;

    draw_text(fb, tx, ty,    "TOASTY SQUADRON",       2, 255, 220,  60);
    draw_text(fb, tx, ty+20, "Made by Pudding Studio",1, 180, 180, 220);

    /* Divider */
    draw_rect(fb, tx, ty+34, pw - (tx - px) - 16, 1, 80, 80, 120, 200);

    /* Item 0 — screensaver toggle */
    int iy = ty + 44;
    const char *label = m->screensaver_on ? "Screensaver auto-start:  [ ON ]"
                                          : "Screensaver auto-start:  [ OFF ]";
    int sel0 = (m->selected == 0);
    if (sel0)
        draw_rect(fb, tx - 2, iy - 2, (int)strlen(label) * 8 + 4, 10, 60, 50, 20, 180);
    draw_text(fb, tx, iy, label, 1,
              sel0 ? 255 : 200, sel0 ? 220 : 200, sel0 ? 60 : 200);

    /* Item 1 — timeout selector (only when screensaver is ON) */
    if (m->screensaver_on) {
        int ty2 = iy + 16;
        int sel1 = (m->selected == 1);

        draw_text(fb, tx, ty2, "Timeout:", 1,
                  sel1 ? 255 : 200, sel1 ? 220 : 200, sel1 ? 60 : 200);

        int ox = tx + 9 * 8 + 4; /* after "Timeout: " */
        for (int i = 0; i < TIMEOUT_COUNT; i++) {
            int active = (i == m->timeout_idx);
            int lw = (int)strlen(TIMEOUT_LABELS[i]) * 8;
            if (active) {
                draw_rect(fb, ox - 3, ty2 - 2, lw + 6, 12,
                          sel1 ? 80 : 50, sel1 ? 65 : 50, sel1 ? 15 : 15, 220);
                draw_text(fb, ox, ty2, TIMEOUT_LABELS[i], 1, 255, 220, 60);
            } else {
                draw_text(fb, ox, ty2, TIMEOUT_LABELS[i], 1, 140, 140, 140);
            }
            ox += lw + 10;
        }
        draw_text(fb, ox, ty2, "min", 1, 140, 140, 140);

        /* left/right hint when timeout row selected */
        if (sel1)
            draw_text(fb, tx, ty2 + 12, "< >:change", 1, 100, 100, 120);
    }

    /* Hint */
    draw_text(fb, tx, py + ph - 18,
              "A:toggle   START/B:close", 1, 140, 140, 160);
}
