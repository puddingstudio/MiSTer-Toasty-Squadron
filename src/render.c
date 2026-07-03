#include <math.h>
#include "render.h"
#include "config.h"

static void draw_layer(FBDev *fb, Scene *sc, int layer)
{
    uint8_t layer_a = (uint8_t)(LAYER_CFG[layer].alpha * 255.0f);

    for (int i = 0; i < sc->poolSize; i++) {
        SpriteInst *inst = &sc->pool[i];
        if (!inst->active || inst->layerIndex != layer) continue;

        SpriteSpec *sp = &sc->specs[inst->specIndex];
        if (sp->numFrames == 0 || !sp->frames) continue;

        SpriteFrame *frame = &sp->frames[inst->frameIndex % sp->numFrames];
        if (!frame->pixels) continue;

        int   pi          = (int)inst->floatPhase % FLOAT_TABLE_SZ;
        float floatOffset = g_float_table[pi] * inst->floatAmp;
        int   drawY       = (int)(inst->y + floatOffset);

        fb_blit(fb, frame->pixels, frame->w, frame->h,
                (int)inst->x, drawY, inst->width, inst->height, layer_a);
    }
}

static void draw_moon(FBDev *fb, Scene *sc)
{
    Moon *m = &sc->moon;
    if (!m->pixels || !m->onScreen) return;
    fb_blit(fb, m->pixels, m->pw, m->ph,
            (int)m->x, (int)m->y, m->width, m->height, 255);
}

void render_bg(FBDev *fb, Scene *sc)
{
    fb_clear(fb);
    draw_layer(fb, sc, 0);
    draw_moon(fb, sc);
    draw_layer(fb, sc, 1);
    draw_layer(fb, sc, 2);
    draw_layer(fb, sc, 3);
}

void render_fg(FBDev *fb, Scene *sc)
{
    draw_layer(fb, sc, 4);
}
