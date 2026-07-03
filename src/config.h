#pragma once

#define SCREEN_W        640
#define TARGET_FPS      60
#define MAX_SPRITES     70
#define NUM_LAYERS      5
#define NUM_SPECS       15
#define FLOAT_TABLE_SZ  360

/* Runtime screen height and pixel aspect ratio — set from framebuffer at startup.
   PAR formula for 4:3 display: (4 * height) / (3 * 640)
     288 lines (PAL CRT)  → 0.60
     240 lines (NTSC CRT) → 0.50
     480 lines (VGA CRT)  → 1.00  */
extern int   g_screen_h;
extern float g_pixel_aspect_r;
#define SCREEN_H       g_screen_h
#define PIXEL_ASPECT_R g_pixel_aspect_r

#define MOON_W          120
#define MOON_H          72
#define MOON_DURATION   120.0f
#define SPAWN_RAMP_TIME 15.0f
#define SPAWN_MAX       0.25f
#define SPAWN_MIN       0.03f
#define FRAME_DURATION  (1.0f / 12.0f)

typedef struct {
    int   size;
    float baseSpeed;
    float speedVar;
    float alpha;
} LayerConfig;

static const LayerConfig LAYER_CFG[NUM_LAYERS] = {
    {  12,  10.0f,  2.0f, 0.20f },  /* 0: far        */
    {  22,  18.0f,  4.0f, 0.60f },  /* 1: middle     */
    {  44,  28.0f,  6.0f, 1.00f },  /* 2: near       */
    {  72,  44.0f,  8.0f, 1.00f },  /* 3: mega       */
    { 160,  72.0f, 14.0f, 1.00f },  /* 4: close mega — renders over OSD */
};

static const int SPEC_FRAMES[NUM_SPECS] = {
    21, 32, 11, 32, 1, 43, 32, 23, 39, 33, 39, 39, 42, 18, 42
};
