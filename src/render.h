#pragma once
#include "fb.h"
#include "sprite.h"
#include "anim.h"

/* Layers 0-3 + moon — call before OSD draw */
void render_bg(FBDev *fb, Scene *sc);

/* Layer 4 (closest mega) — call after OSD draw so it appears on top */
void render_fg(FBDev *fb, Scene *sc);
