#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include "music.h"

static int has_audio_ext(const char *name)
{
    size_t n = strlen(name);
    if (n < 5) return 0;
    const char *ext = name + n - 4;
    return (ext[0] == '.' &&
            ((ext[1]=='m'||ext[1]=='M') && (ext[2]=='p'||ext[2]=='P') && ext[3]=='3')) ||
           (ext[0] == '.' &&
            ((ext[1]=='o'||ext[1]=='O') && (ext[2]=='g'||ext[2]=='G') && (ext[3]=='g'||ext[3]=='G')));
}

typedef struct { const char *key; const char *composer; const char *title; const char *cover; int skip; } TrackMeta;
static const TrackMeta TRACK_DB[] = {
    { "bach",        "J.S. Bach",  "Air on the G String",             "bach",      0 },
    { "beethoven",   "Beethoven",  "Sonata No.15 - Pastoral Andante", "beethoven", 0 },
    { "chopin-noc",  "Chopin",     "Nocturne Op.9 No.2",              "chopin",    0 },
    { "chopin-wal",  "Chopin",     "Waltz",                           "chopin",    0 },
    { "schumann",    "Schumann",   "Kinderszenen Op.15 No.1",         "schumann",  0 },
};

static int set_track_meta(MusicState *m, const char *path)
{
    const char *slash = strrchr(path, '/');
    const char *base  = slash ? slash + 1 : path;

    m->composer[0] = m->title[0] = m->cover_key[0] = '\0';

    for (int i = 0; i < (int)(sizeof(TRACK_DB)/sizeof(TRACK_DB[0])); i++) {
        if (strncmp(base, TRACK_DB[i].key, strlen(TRACK_DB[i].key)) == 0) {
            if (TRACK_DB[i].skip) return 1;  /* signal: skip this track */
            strncpy(m->composer,  TRACK_DB[i].composer, sizeof(m->composer)  - 1);
            strncpy(m->title,     TRACK_DB[i].title,    sizeof(m->title)     - 1);
            strncpy(m->cover_key, TRACK_DB[i].cover,    sizeof(m->cover_key) - 1);
            return 0;
        }
    }
    /* Fallback: use filename as title */
    strncpy(m->title, base, sizeof(m->title) - 1);
    char *dot = strrchr(m->title, '.');
    if (dot) *dot = '\0';
    return 0;
}

static void kill_child(MusicState *m)
{
    if (m->pid > 0) {
        kill(m->pid, SIGKILL);   /* instant — SIGTERM can take 100-200ms */
        waitpid(m->pid, NULL, 0);
        m->pid = -1;
    }
    m->playing = 0;
}

static void play_track(MusicState *m, int idx)
{
    if (m->nfiles == 0) return;
    kill_child(m);

    /* Advance past skipped tracks (max one full cycle) */
    int tries = 0;
    do {
        m->current = ((idx % m->nfiles) + m->nfiles) % m->nfiles;
        idx++;
        tries++;
    } while (set_track_meta(m, m->files[m->current]) && tries < m->nfiles);

    pid_t pid = fork();
    if (pid == 0) {
        int devnull = open("/dev/null", 0);
        if (devnull >= 0) { dup2(devnull, 0); dup2(devnull, 1); dup2(devnull, 2); close(devnull); }
        execlp("mpg123", "mpg123", "--quiet", m->files[m->current], (char *)NULL);
        _exit(1);
    }
    m->pid     = pid;
    m->playing = (pid > 0);
}

int music_start(MusicState *m, const char *dir)
{
    memset(m, 0, sizeof(*m));
    m->pid = -1;
    if (!dir || !*dir) return 0;

    DIR *d = opendir(dir);
    if (!d) return -1;

    struct dirent *de;
    while ((de = readdir(d)) && m->nfiles < MUSIC_MAX_FILES) {
        if (!has_audio_ext(de->d_name)) continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
        m->files[m->nfiles] = strdup(path);
        if (m->files[m->nfiles]) m->nfiles++;
    }
    closedir(d);

    if (m->nfiles == 0) return 0;
    play_track(m, 0);
    return 0;
}

void music_stop(MusicState *m)
{
    kill_child(m);
    for (int i = 0; i < m->nfiles; i++) { free(m->files[i]); m->files[i] = NULL; }
    m->nfiles = 0;
}

void music_next(MusicState *m)
{
    if (m->nfiles == 0) return;
    play_track(m, m->current + 1);
}

void music_prev(MusicState *m)
{
    if (m->nfiles == 0) return;
    play_track(m, m->current - 1);
}

void music_play_toggle(MusicState *m)
{
    if (m->playing) {
        kill_child(m);
    } else {
        play_track(m, m->current);
    }
}

int music_poll(MusicState *m)
{
    if (m->pid <= 0 || !m->playing) return 0;
    if (waitpid(m->pid, NULL, WNOHANG) > 0) {
        m->pid = -1;
        m->playing = 0;
        play_track(m, m->current + 1);  /* auto-advance */
        return 1;
    }
    return 0;
}
