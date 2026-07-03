#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "sprite.h"
#include "config.h"

static float randf(void) {
    return (float)rand() / (float)RAND_MAX;
}

int sprites_load(Scene *sc, const char *assetsDir)
{
    for (int s = 0; s < NUM_SPECS; s++) {
        int nf = SPEC_FRAMES[s];
        sc->specs[s].numFrames = nf;
        sc->specs[s].frames = (SpriteFrame *)calloc(nf, sizeof(SpriteFrame));
        if (!sc->specs[s].frames) {
            fprintf(stderr, "OOM allocating spec %d\n", s);
            return -1;
        }
        for (int f = 0; f < nf; f++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/asset%d/asset%d_%d.png",
                     assetsDir, s + 1, s + 1, f + 1);
            int w, h, ch;
            uint8_t *px = stbi_load(path, &w, &h, &ch, 4);
            if (!px) {
                fprintf(stderr, "Failed to load %s: %s\n", path, stbi_failure_reason());
                return -1;
            }
            sc->specs[s].frames[f].pixels = px;
            sc->specs[s].frames[f].w      = w;
            sc->specs[s].frames[f].h      = h;
        }
    }

    char moonPath[512];
    snprintf(moonPath, sizeof(moonPath), "%s/moon.png", assetsDir);
    int mw, mh, mch;
    uint8_t *mpx = stbi_load(moonPath, &mw, &mh, &mch, 4);
    if (!mpx) {
        fprintf(stderr, "Failed to load moon: %s\n", stbi_failure_reason());
        return -1;
    }
    sc->moon.pixels = mpx;
    sc->moon.pw     = mw;
    sc->moon.ph     = mh;
    sc->moon.width  = MOON_W;
    sc->moon.height = MOON_H;

    return 0;
}

void sprites_free(Scene *sc)
{
    for (int s = 0; s < NUM_SPECS; s++) {
        if (sc->specs[s].frames) {
            for (int f = 0; f < sc->specs[s].numFrames; f++)
                stbi_image_free(sc->specs[s].frames[f].pixels);
            free(sc->specs[s].frames);
            sc->specs[s].frames = NULL;
        }
    }
    if (sc->moon.pixels) {
        stbi_image_free(sc->moon.pixels);
        sc->moon.pixels = NULL;
    }
}

void deck_shuffle(Scene *sc, int layer)
{
    for (int i = 0; i < NUM_SPECS; i++)
        sc->deck[layer][i] = i;
    for (int i = NUM_SPECS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = sc->deck[layer][i];
        sc->deck[layer][i] = sc->deck[layer][j];
        sc->deck[layer][j] = tmp;
    }
    sc->deckIdx[layer] = 0;
}

int deck_pick(Scene *sc, int layer)
{
    if (sc->deckIdx[layer] >= NUM_SPECS)
        deck_shuffle(sc, layer);
    return sc->deck[layer][sc->deckIdx[layer]++];
}

static int pick_layer(void)
{
    float r = randf() * 100.0f;
    if (r < 23.0f) return 0;   /* 23% far                  */
    if (r < 45.0f) return 1;   /* 22% middle               */
    if (r < 63.0f) return 2;   /* 18% near                 */
    if (r < 88.0f) return 3;   /* 25% mega                 */
    return 4;                   /* 12% close mega           */
}

static int mega_overlaps(Scene *sc, float x, float y, int w, int h, int layer)
{
    float buf = w * 0.5f;
    for (int i = 0; i < sc->poolSize; i++) {
        SpriteInst *o = &sc->pool[i];
        if (o->layerIndex != layer) continue;
        int hov = (x - buf) < (o->x + o->width  + buf) &&
                  (x + w   + buf) > (o->x - buf);
        int vov = (y - buf) < (o->y + o->height + buf) &&
                  (y + h   + buf) > (o->y - buf);
        if (hov && vov) return 1;
    }
    return 0;
}

void sprite_spawn(Scene *sc, SpriteInst *inst)
{
    int   li    = inst->layerIndex;
    float base  = 40.0f;
    float extra = (li == 4) ? 150.0f :
                  (li == 3) ? 100.0f :
                  (li == 2) ?  50.0f :
                  (li == 1) ?  25.0f : 0.0f;
    float margin = base + extra;

    /* Close mega always enters from right side — looks better for big sprites */
    float topProb = (li == 4) ? 0.10f :
                    (li == 3) ? 0.85f :
                    (li == 2) ? 0.75f :
                    (li == 1) ? 0.55f : 0.50f;
    int   fromTop = (randf() < topProb);

    int   attempts = (li >= 3) ? 15 : 1;
    float sx = 0, sy = 0;
    for (int a = 0; a < attempts; a++) {
        if (fromTop) {
            sy = -margin - randf() * 60.0f;
            sx = -base + randf() * (SCREEN_W + 2.0f * base + 320.0f) - base;
        } else {
            sx = SCREEN_W + margin + randf() * 60.0f;
            sy = -base + randf() * (SCREEN_H + 2.0f * base + 320.0f) - base;
        }
        if ((li != 3 && li != 4) ||
            !mega_overlaps(sc, sx, sy, inst->width, inst->height, li))
            break;
    }

    float speedRange = LAYER_CFG[li].baseSpeed +
                       (randf() * 2.0f - 1.0f) * LAYER_CFG[li].speedVar;
    float angle = (float)M_PI / 4.0f +
                  (randf() * 7.0f - 3.0f) * (float)M_PI / 180.0f;

    SpriteSpec *sp = &sc->specs[inst->specIndex];

    inst->x          = sx;
    inst->y          = sy;
    inst->vx         = -(float)fabs(speedRange * cos(angle));
    inst->vy         =  (float)fabs(speedRange * sin(angle));
    inst->frameIndex = rand() % sp->numFrames;
    inst->frameTimer = 0.0f;
    inst->floatPhase = randf() * 360.0f;
    inst->floatSpeed = 20.0f + randf() * 40.0f;
    inst->floatAmp   = 2.0f  + randf() * 3.0f;
    inst->active     = 1;
}

void scene_init(Scene *sc, float nowSec)
{
    memset(sc->pool, 0, sizeof(sc->pool));
    sc->poolSize      = 0;
    sc->animStartTime = nowSec;
    sc->lastSpawnTime = nowSec;
    sc->spawnInterval = SPAWN_MAX;

    for (int l = 0; l < NUM_LAYERS; l++)
        deck_shuffle(sc, l);

    for (int i = 0; i < MAX_SPRITES; i++) {
        SpriteInst *inst = &sc->pool[i];
        inst->layerIndex = pick_layer();
        inst->specIndex  = deck_pick(sc, inst->layerIndex);
        int sz           = LAYER_CFG[inst->layerIndex].size;
        inst->width      = sz;
        inst->height     = (int)(sz * PIXEL_ASPECT_R);
        inst->active     = 0;
        inst->x          = (float)(SCREEN_W + 300);
        inst->y          = -300.0f;
    }

    sc->moon.x         = -(float)MOON_W;
    sc->moon.y         = SCREEN_H * 0.3f;
    sc->moon.startTime = nowSec;
    sc->moon.pendingY  = 1;
    sc->moon.onScreen  = 0;
}
