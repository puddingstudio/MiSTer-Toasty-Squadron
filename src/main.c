/*
 * Toasty Squadron — MiSTer FPGA port
 *
 * Run on MiSTer:
 *   cd /media/fat/toasty
 *   TOASTY_ASSETS=assets TOASTY_MUSIC=music ./toasty-squadron-arm
 *
 * Gamepad controls:
 *   B          — exit
 *   A          — cycle clock position
 *   Y          — toggle date
 *   D-pad up   — play / resume music
 *   D-pad down — stop music
 *   D-pad left — previous track
 *   D-pad right— next track
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <linux/input.h>
#include <linux/kd.h>
#include "config.h"
#include "fb.h"
#include "sprite.h"
#include "anim.h"
#include "render.h"
#include "osd.h"
#include "music.h"
#include "id3cover.h"
#include "about.h"

int   g_screen_h      = 288;
float g_pixel_aspect_r = 0.60f;

static volatile int g_running = 1;
static void on_signal(int s) { (void)s; g_running = 0; }

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ── input ──────────────────────────────────────────────────────────────── */

#define MAX_INPUT_FDS 8
static int input_fds[MAX_INPUT_FDS];
static int input_swap_ab[MAX_INPUT_FDS];
static int input_is_virtual[MAX_INPUT_FDS];
static int input_count = 0;

/* Some 8BitDo SNES-style pads (confirmed on the SFC30 via raw evdev capture)
 * report their printed A/B buttons as BTN_SOUTH/BTN_EAST swapped relative to
 * the usual position-based convention (printed A -> BTN_SOUTH, printed B ->
 * BTN_EAST) — the firmware enumerates buttons in legacy SNES ordinal order
 * rather than by physical/compass position, unlike XInput-style pads. Swap
 * them back per-device so A is always "confirm" and B is always "back". */
static int device_needs_ab_swap(const char *name)
{
    return strstr(name, "SFC30") != NULL;
}

/* MiSTer's own OSD layer echoes every physical joystick press as a
 * synthetic keyboard event on a separate virtual device (confirmed via raw
 * evdev capture: pressing a gamepad button also fires an unrelated KEY_*
 * code on this device, per whatever key MiSTer's own default joystick-to-
 * OSD table happens to assign it). Turns out the SFC30's D-pad specifically
 * only ever arrives THROUGH this echo (as KEY_UP/DOWN/LEFT/RIGHT — it has
 * no EV_ABS capability of its own, confirmed via /proc/bus/input/devices),
 * so it can't just be closed outright. Instead only arrow-key codes from it
 * are trusted (see input_poll) — action keys (Enter/Esc/Space/...) are
 * dropped since those collide with keys we bind for real keyboards/pads. */
static int device_is_mister_virtual(const char *name)
{
    return strcmp(name, "MiSTer virtual input") == 0;
}

#define INPUT_QUIT       0x001
#define INPUT_CONFIRM    0x002
#define INPUT_CLOCK      0x004
#define INPUT_DATE       0x008
#define INPUT_MUSIC_PLAY 0x010
#define INPUT_MUSIC_STOP 0x020
#define INPUT_MUSIC_PREV 0x040
#define INPUT_MUSIC_NEXT 0x080
#define INPUT_ABOUT      0x100  /* START — toggle about screen */

static void input_open_all(void)
{
    DIR *d = opendir("/dev/input");
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) && input_count < MAX_INPUT_FDS) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/%s", e->d_name);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        char name[128] = "";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        input_swap_ab[input_count]    = device_needs_ab_swap(name);
        input_is_virtual[input_count] = device_is_mister_virtual(name);
        input_fds[input_count++] = fd;
    }
    closedir(d);
}

static void input_close_all(void)
{
    for (int i = 0; i < input_count; i++) close(input_fds[i]);
    input_count = 0;
}

static int input_poll(void)
{
    struct input_event ev;
    int mask = 0;
    for (int i = 0; i < input_count; i++) {
        while (read(input_fds[i], &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_KEY && ev.value == 1) {
                int code = ev.code;
                if (input_swap_ab[i]) {
                    if      (code == BTN_SOUTH) code = BTN_EAST;
                    else if (code == BTN_EAST)  code = BTN_SOUTH;
                }
                /* MiSTer's own core process exclusively grabs directly-wired
                 * USB joysticks for FPGA/OSD routing (confirmed via
                 * /proc/PID/fd) so the virtual echo device is the ONLY
                 * input path for a wired pad, meaning its confirm/cancel/
                 * nav keys must stay trusted here, same reasoning as
                 * MiSTerFin. */
                if (input_is_virtual[i] &&
                    code != KEY_UP && code != KEY_DOWN &&
                    code != KEY_LEFT && code != KEY_RIGHT &&
                    code != KEY_ENTER && code != KEY_ESC && code != KEY_BACKSPACE) {
                    continue;
                }
                switch (code) {
                /* CONFIRM+CLOCK on EAST ("forward"), QUIT on SOUTH ("back")
                 * — matches MiSTerFin/the standard MiSTer menu's own A/B
                 * convention. CLOCK must NOT share a button with QUIT: QUIT
                 * flips osd.confirm_exit and is checked later in the same
                 * frame, so a button that set both bits would cycle the
                 * clock every time it happened to also cancel the dialog. */
                case BTN_EAST: case KEY_ENTER:
                    mask |= INPUT_CONFIRM | INPUT_CLOCK; break;
                case BTN_SOUTH:
                case KEY_ESC: case KEY_BACKSPACE:
                    mask |= INPUT_QUIT;                 break;
                case BTN_WEST:      mask |= INPUT_DATE;             break;
                case BTN_START: case KEY_SPACE: case KEY_HOME: mask |= INPUT_ABOUT; break;
                case KEY_UP:
                case BTN_DPAD_UP:   mask |= INPUT_MUSIC_PLAY;       break;
                case KEY_DOWN:
                case BTN_DPAD_DOWN: mask |= INPUT_MUSIC_STOP;       break;
                case KEY_LEFT:
                case BTN_DPAD_LEFT: mask |= INPUT_MUSIC_PREV;       break;
                case KEY_RIGHT:
                case BTN_DPAD_RIGHT:mask |= INPUT_MUSIC_NEXT;       break;
                }
            } else if (ev.type == EV_ABS) {
                switch (ev.code) {
                case ABS_HAT0Y:
                    if (ev.value < 0) mask |= INPUT_MUSIC_PLAY;
                    if (ev.value > 0) mask |= INPUT_MUSIC_STOP;
                    break;
                case ABS_HAT0X:
                    if (ev.value < 0) mask |= INPUT_MUSIC_PREV;
                    if (ev.value > 0) mask |= INPUT_MUSIC_NEXT;
                    break;
                case ABS_Y:
                    if (ev.value < -16000) mask |= INPUT_MUSIC_PLAY;
                    if (ev.value >  16000) mask |= INPUT_MUSIC_STOP;
                    break;
                case ABS_X:
                    if (ev.value < -16000) mask |= INPUT_MUSIC_PREV;
                    if (ev.value >  16000) mask |= INPUT_MUSIC_NEXT;
                    break;
                }
            }
        }
    }
    return mask;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    /* Single-instance guard — exit silently if another copy is already running */
    {
        int lfd = open("/tmp/toasty_ss.lock", O_CREAT | O_RDWR, 0600);
        if (lfd >= 0) {
            struct flock fl;
            memset(&fl, 0, sizeof(fl));
            fl.l_type   = F_WRLCK;
            fl.l_whence = SEEK_SET;
            fl.l_len    = 1;
            if (fcntl(lfd, F_SETLK, &fl) < 0) {
                close(lfd);
                return 0;
            }
            /* lfd intentionally kept open — lock auto-released when process exits */
        }
    }

    signal(SIGTERM, on_signal);
    signal(SIGINT,  on_signal);
    srand((unsigned int)time(NULL));

    const char *fb_path    = getenv("TOASTY_FB")     ? getenv("TOASTY_FB")     : "/dev/fb0";
    const char *assets_dir = getenv("TOASTY_ASSETS") ? getenv("TOASTY_ASSETS") : "assets";
    const char *music_dir  = getenv("TOASTY_MUSIC");
    const char *covers_dir = getenv("TOASTY_COVERS") ? getenv("TOASTY_COVERS") : "covers";

    /* Suppress kernel console messages bleeding onto the framebuffer */
    int printk_fd = open("/proc/sys/kernel/printk", O_WRONLY);
    if (printk_fd >= 0) { write(printk_fd, "0\n", 2); close(printk_fd); }

    /* Suppress fbcon text flashes — put VT in graphics mode and disable echo */
    struct termios tty_saved, tty_raw;
    int tty_fd = open("/dev/tty", O_RDWR);
    if (tty_fd >= 0) {
        write(tty_fd, "\033[?25l", 6);           /* hide cursor */
        ioctl(tty_fd, KDSETMODE, KD_GRAPHICS);   /* graphics mode */
        tcgetattr(tty_fd, &tty_saved);
        tty_raw = tty_saved;
        tty_raw.c_lflag &= ~(unsigned)(ECHO | ICANON | ISIG);
        tcsetattr(tty_fd, TCSANOW, &tty_raw);    /* no echo, no line buffering */
    }

    FBDev fb;
    if (fb_open(&fb, fb_path) < 0) {
        fprintf(stderr, "Cannot open framebuffer %s\n", fb_path);
        if (tty_fd >= 0) { ioctl(tty_fd, KDSETMODE, KD_TEXT); close(tty_fd); }
        return 1;
    }
    g_screen_h       = fb.height;
    g_pixel_aspect_r = (float)(4 * fb.height) / (3.0f * 640.0f);
    /* Black out screen immediately — hides the shell terminal text */
    memcpy(fb.mem, fb.back, (size_t)fb.stride * fb.height);
    fprintf(stderr, "Framebuffer: %dx%d stride=%d PAR=%.2f\n",
            fb.width, fb.height, fb.stride, g_pixel_aspect_r);

    input_open_all();

    /* Drain stale input events buffered before launch (e.g. menu button presses) */
    {
        struct input_event ev;
        for (int i = 0; i < input_count; i++)
            while (read(input_fds[i], &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {}
    }

    anim_init_table();

    Scene sc;
    memset(&sc, 0, sizeof(sc));
    if (sprites_load(&sc, assets_dir) < 0) {
        fprintf(stderr, "Failed to load assets from '%s'\n", assets_dir);
        fb_close(&fb);
        input_close_all();
        return 1;
    }

    double t = now_sec();
    scene_init(&sc, (float)t);

    OSDState osd;
    osd_init(&osd, t);

    about_init(assets_dir);

    MusicState music;
    music_start(&music, music_dir);
    if (music.playing) {
        int cw = 0, ch = 0; uint8_t *cpx = NULL;
        if (covers_dir && music.cover_key[0]) {
            char cpath[512];
            snprintf(cpath, sizeof(cpath), "%s/cover-%s.png", covers_dir, music.cover_key);
            cpx = png_load_cover(cpath, &cw, &ch);
        }
        osd_show_track(&osd, t, music.composer, music.title, cpx, cw, ch);
    }

    double last = t;
    int    about_visible    = 0;
    double last_about_press = 0.0;
    const double frame_time = 1.0 / (double)TARGET_FPS;

    while (g_running) {
        int inp = input_poll();
        double now = now_sec();

        /* START toggles about screen — 300ms debounce */
        if ((inp & INPUT_ABOUT) && (now - last_about_press > 0.3)) {
            about_visible    = !about_visible;
            osd.confirm_exit = 0;
            last_about_press = now;
        }
        if (about_visible) {
            if (inp & INPUT_QUIT) about_visible = 0;
            if (inp & INPUT_CONFIRM) about_start_install();
            goto frame_end;
        }

        if (inp & INPUT_QUIT) {
            if (osd.confirm_exit) {
                osd.confirm_exit = 0;   /* B cancels */
            } else {
                osd.confirm_exit = 1;   /* B opens dialog */
            }
        }
        if (osd.confirm_exit) {
            if (inp & INPUT_CONFIRM) break;  /* A confirms exit */
            goto frame_end;
        }

        if (inp & INPUT_CLOCK) {
            osd.clock_pos = (ClockPos)((osd.clock_pos + 1) % CLOCK_POS_COUNT);
            osd.hints_visible = 1;
            osd.hints_hide_at = now + 8.0;
        }
        if (inp & INPUT_DATE) {
            osd.date_visible ^= 1;
            osd.hints_visible = 1;
            osd.hints_hide_at = now + 8.0;
        }

        /* music controls — load cover on track change */
        {
            int track_changed = 0;
            if (inp & INPUT_MUSIC_PLAY) { music_play_toggle(&music); track_changed = 1; }
            if (inp & INPUT_MUSIC_STOP) { music_play_toggle(&music); track_changed = 1; }
            if (inp & INPUT_MUSIC_PREV) { music_prev(&music);        track_changed = 1; }
            if (inp & INPUT_MUSIC_NEXT) { music_next(&music);        track_changed = 1; }
            if (track_changed) {
                if (!music.playing) {
                    /* stopped — clear NP banner and cover immediately */
                    osd_free_cover(&osd);
                    osd.np_composer[0] = '\0';
                    osd.np_title[0]    = '\0';
                    osd.np_hide_at     = 0;
                } else {
                    int cw = 0, ch = 0; uint8_t *cpx = NULL;
                    if (covers_dir && music.cover_key[0]) {
                        char cpath[512];
                        snprintf(cpath, sizeof(cpath), "%s/cover-%s.png", covers_dir, music.cover_key);
                        cpx = png_load_cover(cpath, &cw, &ch);
                    }
                    osd_show_track(&osd, now, music.composer, music.title, cpx, cw, ch);
                }
            }
        }

        /* auto-advance when track ends naturally */
        if (music_poll(&music)) {
            int cw = 0, ch = 0; uint8_t *cpx = NULL;
            if (covers_dir && music.cover_key[0]) {
                char cpath[512];
                snprintf(cpath, sizeof(cpath), "%s/cover-%s.png", covers_dir, music.cover_key);
                cpx = png_load_cover(cpath, &cw, &ch);
            }
            osd_show_track(&osd, now, music.composer, music.title, cpx, cw, ch);
        }

        frame_end:;
        double dt = now - last;
        last = now;
        if (dt > 0.1) dt = 0.1;

        osd_update(&osd, now);
        anim_update(&sc, (float)now, (float)dt);

        if (about_visible) {
            about_draw(&fb, 1);
        } else {
            render_bg(&fb, &sc);
            osd_draw(&osd, &fb);     /* clock/NP behind sprites */
            render_fg(&fb, &sc);     /* layer-4 sprites over clock */
            osd_draw_confirm(&osd, &fb); /* confirm dialog always on top */
        }
        fb_flip(&fb);

        double elapsed = now_sec() - last;
        if (elapsed < frame_time) {
            useconds_t us = (useconds_t)((frame_time - elapsed) * 1e6);
            if (us > 0) usleep(us);
        }
    }

    osd_free_cover(&osd);
    about_free();
    music_stop(&music);
    fb_clear(&fb);
    fb_flip(&fb);
    sprites_free(&sc);
    fb_close(&fb);
    input_close_all();
    int restore_fd = open("/proc/sys/kernel/printk", O_WRONLY);
    if (restore_fd >= 0) { write(restore_fd, "7\n", 2); close(restore_fd); }
    if (tty_fd >= 0) {
        tcsetattr(tty_fd, TCSANOW, &tty_saved);  /* restore echo / line mode */
        ioctl(tty_fd, KDSETMODE, KD_TEXT);
        write(tty_fd, "\033[?25h", 6);            /* restore cursor */
        close(tty_fd);
    }
    return 0;
}
