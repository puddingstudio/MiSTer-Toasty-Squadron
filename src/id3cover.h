#pragma once
#include <stdint.h>

/* Extract embedded cover art from an MP3's ID3v2 APIC frame.
   Returns RGBA pixel buffer (malloc'd, caller must free) or NULL.
   Sets *w and *h to the decoded image dimensions. */
uint8_t *id3_load_cover(const char *mp3_path, int *w, int *h);
uint8_t *png_load_cover(const char *path, int *w, int *h);
