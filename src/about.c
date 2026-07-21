#include "about.h"
#include "fb.h"
#include "config.h"
#include "stb_image.h"
#include "font8x8.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

/* ── Update check ─────────────────────────────────────────────────────────── */

typedef enum { UPD_CHECKING, UPD_OK, UPD_AVAILABLE, UPD_FAILED } UpdateState;

static UpdateState     g_upd_state  = UPD_CHECKING;
static char            g_upd_latest[32] = {0};
static pthread_mutex_t g_upd_mutex  = PTHREAD_MUTEX_INITIALIZER;

static void *update_check_thread(void *arg)
{
    (void)arg;
    FILE *f = popen(
        "curl -sfk --max-time 8 "
        "https://api.github.com/repos/puddingstudio/MiSTer-Toasty-Squadron/releases/latest "
        "2>/dev/null", "r");
    if (!f) {
        pthread_mutex_lock(&g_upd_mutex);
        g_upd_state = UPD_FAILED;
        pthread_mutex_unlock(&g_upd_mutex);
        return NULL;
    }
    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, f);
    pclose(f);

    char *p = strstr(buf, "\"tag_name\"");
    if (!p) {
        pthread_mutex_lock(&g_upd_mutex);
        g_upd_state = UPD_FAILED;
        pthread_mutex_unlock(&g_upd_mutex);
        return NULL;
    }
    p = strchr(p, ':'); if (!p) goto fail;
    p++;
    while (*p == ' ' || *p == '"') p++;
    char tag[32] = {0};
    int i = 0;
    while (*p && *p != '"' && i < 31) tag[i++] = *p++;

    pthread_mutex_lock(&g_upd_mutex);
    strncpy(g_upd_latest, tag, sizeof(g_upd_latest) - 1);
    g_upd_state = (strcmp(tag, APP_VERSION) == 0) ? UPD_OK : UPD_AVAILABLE;
    pthread_mutex_unlock(&g_upd_mutex);
    return NULL;

fail:
    pthread_mutex_lock(&g_upd_mutex);
    g_upd_state = UPD_FAILED;
    pthread_mutex_unlock(&g_upd_mutex);
    return NULL;
}

/* ── Install ──────────────────────────────────────────────────────────────── */

typedef enum { INST_IDLE, INST_DOWNLOADING, INST_DONE, INST_FAILED } InstallState;

static InstallState    g_inst_state = INST_IDLE;
static pthread_mutex_t g_inst_mutex = PTHREAD_MUTEX_INITIALIZER;

static void set_inst(InstallState s)
{
    pthread_mutex_lock(&g_inst_mutex);
    g_inst_state = s;
    pthread_mutex_unlock(&g_inst_mutex);
}

static void *install_thread(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&g_upd_mutex);
    char tag[32];
    strncpy(tag, g_upd_latest, sizeof(tag) - 1);
    pthread_mutex_unlock(&g_upd_mutex);

    /* Download zip */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -Lsk --max-time 120 "
        "https://github.com/puddingstudio/MiSTer-Toasty-Squadron/releases/download/%s/toasty-squadron-%s.zip "
        "-o /tmp/toasty-update.zip 2>/dev/null",
        tag, tag);
    if (system(cmd) != 0) { set_inst(INST_FAILED); return NULL; }

    /* Extract */
    system("rm -rf /tmp/toasty-update/");
    if (system("unzip -q -o /tmp/toasty-update.zip -d /tmp/toasty-update/ 2>/dev/null") != 0)
        { set_inst(INST_FAILED); return NULL; }

    /* Copy assets, covers, music immediately */
    system("cp -r /tmp/toasty-update/toasty/assets/. /media/fat/toasty/assets/ 2>/dev/null");
    system("cp -r /tmp/toasty-update/toasty/covers/. /media/fat/toasty/covers/ 2>/dev/null");
    system("cp -r /tmp/toasty-update/toasty/music/.  /media/fat/toasty/music/  2>/dev/null");

    /* Write update script — applied by ToastySquadron.sh on next launch */
    FILE *f = fopen("/tmp/toasty_apply_update.sh", "w");
    if (f) {
        fprintf(f, "#!/bin/bash\n");
        fprintf(f, "cp /tmp/toasty-update/toasty/toasty-squadron-arm /media/fat/toasty/toasty-squadron-arm\n");
        fprintf(f, "chmod +x /media/fat/toasty/toasty-squadron-arm\n");
        fprintf(f, "rm -rf /tmp/toasty-update/ /tmp/toasty-update.zip\n");
        fclose(f);
        chmod("/tmp/toasty_apply_update.sh", 0755);
    }

    set_inst(INST_DONE);
    return NULL;
}

void about_start_install(void)
{
    pthread_mutex_lock(&g_inst_mutex);
    if (g_inst_state != INST_IDLE) { pthread_mutex_unlock(&g_inst_mutex); return; }
    g_inst_state = INST_DOWNLOADING;
    pthread_mutex_unlock(&g_inst_mutex);

    pthread_t tid;
    pthread_create(&tid, NULL, install_thread, NULL);
    pthread_detach(tid);
}

int about_install_done(void)
{
    pthread_mutex_lock(&g_inst_mutex);
    InstallState s = g_inst_state;
    pthread_mutex_unlock(&g_inst_mutex);
    return (s == INST_DONE) ? 1 : 0;
}

/* ── Drawing ──────────────────────────────────────────────────────────────── */

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

/* "Flying through stars" background, same as MiSTerFin's About screen —
 * each star has a depth (z) that shrinks every frame; projecting x/y by
 * 1/z makes it appear to accelerate toward the viewer, respawning at max
 * depth once it passes the camera or drifts off-screen. */
#define STAR_COUNT 40
typedef struct { float x, y, z; } Star;
static Star g_stars[STAR_COUNT];
static int  g_stars_init = 0;

static float star_frand_pm1(void) { return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f; }

static void star_respawn(Star *s)
{
    s->x = star_frand_pm1();
    s->y = star_frand_pm1();
    s->z = 1.0f;
}

static void draw_starfield(FBDev *fb)
{
    if (!g_stars_init) {
        g_stars_init = 1;
        for (int i = 0; i < STAR_COUNT; i++) {
            star_respawn(&g_stars[i]);
            g_stars[i].z = 0.05f + ((float)rand() / (float)RAND_MAX) * 0.95f;
        }
    }
    for (int i = 0; i < STAR_COUNT; i++) {
        Star *s = &g_stars[i];
        s->z -= 0.012f;
        if (s->z <= 0.05f) star_respawn(s);

        float scale = 1.0f / s->z;
        int sx = (int)(fb->width  / 2 + s->x * scale * (fb->width  / 4));
        int sy = (int)(fb->height / 2 + s->y * scale * (fb->height / 4));
        if (sx < 0 || sx >= fb->width || sy < 0 || sy >= fb->height) { star_respawn(s); continue; }

        int size = scale > 2.2f ? 2 : 1;
        uint8_t bright = scale > 1.4f ? 255 : 150;
        fb_fill_rect_alpha(fb, sx, sy, size, size, bright, bright, bright, 255);
    }
}

/* Waffle (asset15 — the biggest sprite in the deck, 320x320 vs. every
 * other sprite's 128x128, confirmed by checking each PNG's header) stands
 * in for the old about.png photo, animated through its full frame set
 * instead of a static image. Loaded directly here (not through the main
 * Scene/sprite pool in sprite.c) since the About screen doesn't need any
 * of that pool's physics/floating logic, just a plain looping blit. */
#define WAFFLE_SPEC_INDEX (NUM_SPECS - 1)
#define WAFFLE_DISPLAY_SIZE 200

static uint8_t *g_waffle_px[64];
static int      g_waffle_w[64], g_waffle_h[64];
static int      g_waffle_n = 0;
static int      g_waffle_frame = 0;
static int      g_waffle_tick = 0;

void about_init(const char *assets_dir)
{
    g_waffle_n = SPEC_FRAMES[WAFFLE_SPEC_INDEX];
    if (g_waffle_n > 64) g_waffle_n = 64;
    for (int i = 0; i < g_waffle_n; i++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/asset%d/asset%d_%d.png",
                 assets_dir, WAFFLE_SPEC_INDEX + 1, WAFFLE_SPEC_INDEX + 1, i + 1);
        int ch;
        g_waffle_px[i] = stbi_load(path, &g_waffle_w[i], &g_waffle_h[i], &ch, 4);
    }

    g_upd_state  = UPD_CHECKING;
    g_inst_state = INST_IDLE;
    pthread_t tid;
    pthread_create(&tid, NULL, update_check_thread, NULL);
    pthread_detach(tid);
}

void about_free(void)
{
    for (int i = 0; i < g_waffle_n; i++)
        if (g_waffle_px[i]) { stbi_image_free(g_waffle_px[i]); g_waffle_px[i] = NULL; }
    g_waffle_n = 0;
}

void about_draw(FBDev *fb)
{
    fb_clear(fb);
    draw_starfield(fb);

    static const char TITLE[] = "Toasty Squadron";
    static const char LINE1[] = "made over the weekends at pudding";
    static const char LINE2[] = "https://pudding.studio";

    const int ts  = 2;
    const int s1  = 1;
    const int tch = 8 * ts;
    const int sch = 8 * s1;
    const int gap = 8;
    const int lsp = 4;

    int have_waffle = (g_waffle_n > 0 && g_waffle_px[g_waffle_frame]);
    int img_w = have_waffle ? WAFFLE_DISPLAY_SIZE : 0;
    /* CRT non-square pixels stretch a plain square vertically — every other
     * sprite in the scene corrects for this by scaling its height down by
     * PIXEL_ASPECT_R (see sprite.c's own inst->height calc); this square
     * waffle blit needs the same correction or it reads as "elongated" on
     * real hardware (confirmed by the user), even though it looks fine
     * rendered off-device where pixels are actually square. */
    int img_h = (int)(img_w * PIXEL_ASPECT_R);

    int total_h = img_h + (img_h ? gap : 0) + tch + 6 + sch + lsp + sch;
    int dx = (fb->width - img_w) / 2;
    /* Centering across the FULL screen height sat too low (crowding the
     * bottom-left version/update line below) — same fix as MiSTerFin's own
     * About screen: reserve room for that footer first, then center the
     * sprite+text block in what's left above it, nudged down a touch so
     * it doesn't hug the top. */
    int avail_bottom = fb->height - 8 - 20 - 14 - 6;
    int dy = (avail_bottom - total_h) / 2 + 12;
    if (dy < 8) dy = 8;

    if (have_waffle) {
        /* 12fps, same cadence as the main scene's own sprite animation
         * (FRAME_DURATION in config.h) — TARGET_FPS/5 ≈ 12. */
        if (++g_waffle_tick >= 5) {
            g_waffle_tick = 0;
            g_waffle_frame = (g_waffle_frame + 1) % g_waffle_n;
        }
        fb_blit(fb, g_waffle_px[g_waffle_frame], g_waffle_w[g_waffle_frame], g_waffle_h[g_waffle_frame],
                dx, dy, img_w, img_h, 255);
    }

    int tty = dy + img_h + gap;
    int ty1 = tty + tch + 6;
    int ty2 = ty1 + sch + lsp;
    int ttx = (fb->width - (int)strlen(TITLE) * 8 * ts) / 2;
    int tx1 = (fb->width - (int)strlen(LINE1) * 8 * s1) / 2;
    int tx2 = (fb->width - (int)strlen(LINE2) * 8 * s1) / 2;

    draw_text(fb, ttx, tty, TITLE, ts, 0xFF, 0xE0, 0x40, 255);
    draw_text(fb, tx1, ty1, LINE1, s1, 0xCC, 0xCC, 0xCC, 255);
    draw_text(fb, tx2, ty2, LINE2, s1, 0xFF, 0xC0, 0x40, 255);

    /* ── bottom-left: version + update status ── */
    pthread_mutex_lock(&g_upd_mutex);
    UpdateState us = g_upd_state;
    char latest[32];
    strncpy(latest, g_upd_latest, sizeof(latest));
    pthread_mutex_unlock(&g_upd_mutex);

    pthread_mutex_lock(&g_inst_mutex);
    InstallState is = g_inst_state;
    pthread_mutex_unlock(&g_inst_mutex);

    const int lx    = 32;
    const int safe_y = fb->height - 8 - 20;

    char installed[48];
    snprintf(installed, sizeof(installed), "%s Installed", APP_VERSION);

    if (is == INST_DOWNLOADING) {
        static int dot_frame = 0;
        dot_frame++;
        const char *dots = (dot_frame / 20 % 3 == 0) ? "." : (dot_frame / 20 % 3 == 1) ? ".." : "...";
        char dl[48];
        snprintf(dl, sizeof(dl), "downloading %s", dots);
        draw_text(fb, lx, safe_y - 14, installed, 1, 120, 120, 120, 180);
        draw_text(fb, lx, safe_y, dl, 1, 80, 180, 80, 220);
    } else if (is == INST_DONE) {
        draw_text(fb, lx, safe_y - 14, installed, 1, 120, 120, 120, 180);
        draw_text(fb, lx, safe_y, "update installed   restart app to apply", 1, 80, 180, 80, 220);
    } else if (is == INST_FAILED) {
        draw_text(fb, lx, safe_y - 14, installed, 1, 120, 120, 120, 180);
        draw_text(fb, lx, safe_y, "download failed", 1, 200, 60, 60, 220);
    } else if (us == UPD_AVAILABLE) {
        draw_text(fb, lx, safe_y - 14, installed, 1, 120, 120, 120, 180);
        char upd[64];
        snprintf(upd, sizeof(upd), "%s available   A: install", latest);
        draw_text(fb, lx, safe_y, upd, 1, 220, 150, 40, 220);
    } else {
        draw_text(fb, lx, safe_y, installed, 1, 120, 120, 120, 180);
    }
}
