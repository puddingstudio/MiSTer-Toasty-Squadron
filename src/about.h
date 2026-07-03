#pragma once
#include "fb.h"

void about_init(const char *assets_dir);
void about_free(void);
void about_draw(FBDev *fb);
void about_start_install(void);
int  about_install_done(void);
