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

static uint8_t *g_px = NULL;
static int      g_w  = 0;
static int      g_h  = 0;

void about_init(const char *assets_dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/about.png", assets_dir);
    int ch;
    g_px = stbi_load(path, &g_w, &g_h, &ch, 4);

    g_upd_state  = UPD_CHECKING;
    g_inst_state = INST_IDLE;
    pthread_t tid;
    pthread_create(&tid, NULL, update_check_thread, NULL);
    pthread_detach(tid);
}

void about_free(void)
{
    if (g_px) { stbi_image_free(g_px); g_px = NULL; }
    g_w = g_h = 0;
}

void about_draw(FBDev *fb)
{
    for (int y = 0; y < fb->height; y++) {
        uint32_t *row = (uint32_t *)(fb->back + y * fb->stride);
        for (int x = 0; x < fb->width; x++)
            row[x] = 0xFFD4D4D4;
    }

    if (!g_px || g_w <= 0 || g_h <= 0) return;

    static const char TITLE[] = "Toasty Squadron";
    static const char LINE1[] = "made over the weekends at pudding";
    static const char LINE2[] = "https://pudding.studio";

    const int ts  = 2;
    const int s1  = 1;
    const int tch = 8 * ts;
    const int sch = 8 * s1;
    const int gap = 8;
    const int lsp = 4;

    int total_h = g_h + gap + tch + 6 + sch + lsp + sch;
    int dx = (fb->width - g_w) / 2;
    int dy = (fb->height - total_h) / 2;

    fb_blit(fb, g_px, g_w, g_h, dx, dy, g_w, g_h, 255);

    int tty = dy + g_h + gap;
    int ty1 = tty + tch + 6;
    int ty2 = ty1 + sch + lsp;
    int ttx = (fb->width - (int)strlen(TITLE) * 8 * ts) / 2;
    int tx1 = (fb->width - (int)strlen(LINE1) * 8 * s1) / 2;
    int tx2 = (fb->width - (int)strlen(LINE2) * 8 * s1) / 2;

    draw_text(fb, ttx, tty, TITLE, ts, 40,  40,  40, 230);
    draw_text(fb, tx1, ty1, LINE1, s1, 80,  80,  80, 200);
    draw_text(fb, tx2, ty2, LINE2, s1, 30,  80, 180, 210);

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
