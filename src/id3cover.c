#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "id3cover.h"
#include "stb_image.h"  /* implementation is in sprite.c */

uint8_t *png_load_cover(const char *path, int *w, int *h)
{
    *w = 0; *h = 0;
    int ch;
    return stbi_load(path, w, h, &ch, 4);
}

static uint32_t synchsafe(const uint8_t *b)
{
    return ((uint32_t)(b[0] & 0x7F) << 21) |
           ((uint32_t)(b[1] & 0x7F) << 14) |
           ((uint32_t)(b[2] & 0x7F) <<  7) |
            (uint32_t)(b[3] & 0x7F);
}

static uint32_t be32(const uint8_t *b)
{
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
           ((uint32_t)b[2] <<  8) |  (uint32_t)b[3];
}

uint8_t *id3_load_cover(const char *mp3_path, int *w, int *h)
{
    *w = 0; *h = 0;
    FILE *f = fopen(mp3_path, "rb");
    if (!f) return NULL;

    uint8_t hdr[10];
    if (fread(hdr, 1, 10, f) != 10 || memcmp(hdr, "ID3", 3) != 0) {
        fclose(f);
        return NULL;
    }

    int  ver        = hdr[3];      /* ID3v2 major version: 3 or 4 */
    uint32_t tag_sz = synchsafe(hdr + 6);

    uint8_t *apic_data = NULL;
    uint32_t apic_len  = 0;

    uint32_t pos = 0;
    while (pos + 10 <= tag_sz) {
        uint8_t frame[10];
        if (fread(frame, 1, 10, f) != 10) break;

        uint32_t frame_sz = (ver >= 4) ? synchsafe(frame + 4) : be32(frame + 4);
        pos += 10;

        if (frame_sz == 0 || pos + frame_sz > tag_sz) break;

        if (memcmp(frame, "APIC", 4) == 0) {
            apic_data = (uint8_t *)malloc(frame_sz);
            if (apic_data) {
                apic_len = (uint32_t)fread(apic_data, 1, frame_sz, f);
            }
            break;
        }

        fseek(f, (long)frame_sz, SEEK_CUR);
        pos += frame_sz;
    }
    fclose(f);

    if (!apic_data || apic_len < 4) { free(apic_data); return NULL; }

    /* APIC layout:
       [0]     text encoding
       [1..]   MIME type (null-terminated), e.g. "image/jpeg"
       [..]    picture type (1 byte)
       [..]    description (null-terminated)
       [..]    image data */
    uint32_t off = 1;
    /* skip MIME */
    while (off < apic_len && apic_data[off]) off++;
    off++;  /* null terminator */
    /* skip picture type */
    if (off >= apic_len) { free(apic_data); return NULL; }
    off++;
    /* skip description */
    while (off < apic_len && apic_data[off]) off++;
    off++;

    if (off >= apic_len) { free(apic_data); return NULL; }

    int img_w, img_h, ch;
    uint8_t *pixels = stbi_load_from_memory(
        apic_data + off, (int)(apic_len - off),
        &img_w, &img_h, &ch, 4);
    free(apic_data);

    if (!pixels) return NULL;
    *w = img_w;
    *h = img_h;
    return pixels;
}
