#pragma once
#include "sprite.h"

extern float g_float_table[360];

void anim_init_table(void);
void anim_update(Scene *sc, float nowSec, float dt);
