#pragma once
#include <sys/types.h>

#define MUSIC_MAX_FILES 32

typedef struct {
    pid_t  pid;
    char  *files[MUSIC_MAX_FILES];
    int    nfiles;
    int    current;
    int    playing;
    char   composer[64];     /* e.g. "Bach" */
    char   title[96];        /* e.g. "Air on G String" */
    char   cover_key[32];    /* e.g. "bach" — used to find cover-*.png */
} MusicState;

int  music_start(MusicState *m, const char *dir);
void music_stop(MusicState *m);
void music_next(MusicState *m);
void music_prev(MusicState *m);
void music_play_toggle(MusicState *m);
int  music_poll(MusicState *m);   /* returns 1 if track changed (auto-advance) */
