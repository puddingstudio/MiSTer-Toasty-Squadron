#include <math.h>
#include <stdlib.h>
#include "anim.h"
#include "config.h"

float g_float_table[FLOAT_TABLE_SZ];

void anim_init_table(void)
{
    for (int i = 0; i < FLOAT_TABLE_SZ; i++)
        g_float_table[i] = (float)sin(i * M_PI / 180.0);
}

static float ease_inout_cubic(float t)
{
    if (t < 0.5f)
        return 4.0f * t * t * t;
    float f = 2.0f * t - 2.0f;
    return f * f * f * 0.5f + 1.0f;
}

static void update_moon(Scene *sc, float nowSec, float dt)
{
    Moon *m = &sc->moon;
    if (!m->pixels) return;
    if (dt == 0.0f) return;

    float elapsed = nowSec - m->startTime;
    if (elapsed >= MOON_DURATION) {
        m->startTime = nowSec;
        m->x         = -(float)m->width;
        m->pendingY  = 1;
        m->onScreen  = 0;
        elapsed      = 0.0f;
    }

    float progress = elapsed / MOON_DURATION;
    float startX   = -(float)m->width;
    float endX     = (float)SCREEN_W;
    m->x = startX + (endX - startX) * progress;

    if (m->pendingY) {
        float minY = SCREEN_H * 0.20f;
        float maxY = SCREEN_H * 0.60f;
        m->y       = minY + ((float)rand() / (float)RAND_MAX) * (maxY - minY);
        m->pendingY = 0;
    }

    m->onScreen = (m->x > -(float)m->width) && (m->x < (float)SCREEN_W);
}

static void update_spawn(Scene *sc, float nowSec)
{
    if (sc->poolSize >= MAX_SPRITES) return;

    float timeSinceStart = nowSec - sc->animStartTime;
    float progress       = timeSinceStart / SPAWN_RAMP_TIME;
    if (progress > 1.0f) progress = 1.0f;
    float eased = ease_inout_cubic(progress);
    sc->spawnInterval = SPAWN_MAX - eased * (SPAWN_MAX - SPAWN_MIN);

    if ((nowSec - sc->lastSpawnTime) >= sc->spawnInterval) {
        SpriteInst *inst = &sc->pool[sc->poolSize];
        sprite_spawn(sc, inst);
        sc->poolSize++;
        sc->lastSpawnTime = nowSec;
    }
}

void anim_update(Scene *sc, float nowSec, float dt)
{
    update_spawn(sc, nowSec);
    update_moon(sc, nowSec, dt);

    if (dt <= 0.0f) return;

    for (int i = 0; i < sc->poolSize; i++) {
        SpriteInst *inst = &sc->pool[i];
        if (!inst->active) continue;

        SpriteSpec *sp = &sc->specs[inst->specIndex];

        inst->frameTimer += dt;
        if (inst->frameTimer >= FRAME_DURATION) {
            inst->frameTimer -= FRAME_DURATION;
            inst->frameIndex  = (inst->frameIndex + 1) % sp->numFrames;
        }

        inst->x          += inst->vx * dt;
        inst->y          += inst->vy * dt;
        inst->floatPhase += inst->floatSpeed * dt;
        if (inst->floatPhase >= 360.0f)
            inst->floatPhase -= 360.0f;

        if (inst->x < -(float)inst->width ||
            inst->y > (float)(SCREEN_H + inst->height))
        {
            sprite_spawn(sc, inst);
        }
    }
}
