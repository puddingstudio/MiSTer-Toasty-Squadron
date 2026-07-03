#pragma once
#include <stdint.h>
#include "config.h"

typedef struct {
    uint8_t *pixels;  /* RGBA8888, w*h*4 bytes */
    int      w, h;
} SpriteFrame;

typedef struct {
    SpriteFrame *frames;
    int          numFrames;
} SpriteSpec;

typedef struct {
    uint8_t *pixels;   /* RGBA8888, pw*ph*4 bytes */
    int      pw, ph;   /* original pixel dimensions */
    float    x, y;
    int      width, height;  /* display dimensions */
    float    startTime;
    int      pendingY;
    int      onScreen;
} Moon;

typedef struct {
    int   specIndex;
    int   layerIndex;
    int   frameIndex;
    float frameTimer;
    float x, y;
    float vx, vy;
    float floatPhase;
    float floatSpeed;
    float floatAmp;
    int   width, height;
    int   active;
} SpriteInst;

typedef struct {
    SpriteSpec specs[NUM_SPECS];
    SpriteInst pool[MAX_SPRITES];
    int        poolSize;
    int        deck[NUM_LAYERS][NUM_SPECS];
    int        deckIdx[NUM_LAYERS];
    Moon       moon;
    float      animStartTime;
    float      lastSpawnTime;
    float      spawnInterval;
} Scene;

int  sprites_load(Scene *sc, const char *assetsDir);
void sprites_free(Scene *sc);
void scene_init(Scene *sc, float nowSec);
void deck_shuffle(Scene *sc, int layer);
int  deck_pick(Scene *sc, int layer);
void sprite_spawn(Scene *sc, SpriteInst *inst);
