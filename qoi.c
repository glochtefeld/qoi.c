#include "qoi.h"
#include <byteswap.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char *statuses[] = {
    "QOI_OK",
    "QOI_BAD_FPATH",
    "QOI_CANNOT_MALLOC",
    "QOI_FREAD_ERR",
    "QOI_DECODE_ERR",
    "QOI_NULLPTR_PASSED",
    "QOI_BAD_HEADER",
};
static const char qoi_padding[] = { 0, 0, 0, 0, 0, 0, 0, 1 };

// Big endian; 0x01020304 should become 0x01 0x02 0x03 0x04
static void write_i32(uint32_t data, char *buf, int *idx) {
    buf[(*idx)++] = data >> 24;
    buf[(*idx)++] = (data >> 16) & 0xFF;
    buf[(*idx)++] = (data >> 8) & 0xFF;
    buf[(*idx)++] = data & 0xFF;
}
static inline int between(int a, int x, int b) {
    return a < x && x < b;
}
static inline int diff_lt_4(int x) { return between(-3, x, 2); }
static inline int diff_lt_16(int x) { return between(-8, x, 8); }
static inline int diff_lt_32(int x) { return between(-33, x, 32); }

static inline int qoi_minor_diff(int dr, int dg, int db) {
    return diff_lt_4(dr) && diff_lt_4(dg) && diff_lt_4(db);
}
static inline int qoi_luma_diff(int dr_dg, int dg, int db_dg) {
    return diff_lt_16(dr_dg) && diff_lt_32(dg) && diff_lt_16(db_dg);
}
qoi_status qoi_write(const char *path, const qoi_image *image) {
    FILE *outfile = fopen(path, "w");
    if (outfile == NULL) return QOI_BAD_FPATH; 

    uint32_t outchars;
    qoi_status st;
    uint8_t *encoded = qoi_encode(image, &outchars, &st);
    if (st != QOI_OK) {
        fclose(outfile);
        return st;
    }
    fwrite(encoded, sizeof(uint8_t), outchars, outfile);
    fclose(outfile);
    free(encoded);
}
uint8_t *qoi_encode(const qoi_image *image, uint32_t *outlen, qoi_status *out_status) {
    #define NEXTOUT output[(*outlen)++]
    if (image == NULL) {
        *out_status = QOI_NULLPTR_PASSED;
        return NULL;
    }
    if (image->header->width == 0 || image->header->height == 0
        || image->header->channels < 3 || image->header->channels > 4
        || image->header->colorspace > 1) {
        *out_status = QOI_BAD_HEADER;
        return NULL;
    }

    char *pixels = image->pixels;
    uint32_t px_len = image->header->width * image->header->height * image->header->channels;
    uint32_t last_px_idx = px_len - image->header->channels;
    int is_rgba = image->header->channels == 4;

    int maxlen = QOI_HDR_LEN 
        + image->header->width * image->header->height * (image->header->channels + 1) 
        + sizeof(qoi_padding);
    uint8_t *output = malloc(sizeof(uint8_t) * maxlen);
    if (output == NULL) {
        *out_status = QOI_CANNOT_MALLOC;
        return NULL;
    }
    *outlen = 0;

    // write header data
    write_i32(QOI_MAGIC, output, outlen);
    write_i32(image->header->width, output, outlen);
    write_i32(image->header->height, output, outlen);
    NEXTOUT = image->header->channels;
    NEXTOUT = image->header->colorspace;

    // prep lookup table and run variables
    int runlen = 0;
    qoi_rgba lookup[64];
    memset(lookup, 0U, sizeof(lookup));

    // write actual data
    qoi_rgba prev = {.v = 0xFF000000 }; // ABGR
    qoi_rgba current = prev;
    for (int px_idx=0; px_idx < px_len; px_idx += image->header->channels) {
        current.rgba.r = pixels[px_idx+0];       
        current.rgba.g = pixels[px_idx+1];       
        current.rgba.b = pixels[px_idx+2];       
        if (is_rgba)
            current.rgba.a = pixels[px_idx+3];
        
        printf("current(previous): 0x%08X(0x%08X)\n", __bswap_32(current.v), __bswap_32(prev.v));

        if (current.v == prev.v) {
            printf("found run\n");
            runlen++;
            if (runlen == 62 || px_idx == last_px_idx) {
                NEXTOUT = QOI_OP_RUN | (runlen - 1);
                runlen = 0;
            }
            // Look, we can't afford all these levels of indentation! In this economy?
            goto nextloop; 
        }

        if (runlen > 0) { // no matter what, we can reset the run length
            printf("Ending run\n");
            NEXTOUT = QOI_OP_RUN | (runlen - 1);
            runlen = 0;
        }

        // See if the value occurred recently enough to index
        int index = QOI_HASH(current) % 64;
        printf("hashed: %d\n", index);
        if (lookup[index].v == current.v) {
            printf("found previously indexed pixel\n");
            NEXTOUT = QOI_OP_INDEX | index;
            goto nextloop;
        } 
        lookup[index] = current;

        if (prev.rgba.a == current.rgba.a) {
            // Calculate differences
            int dr = prev.rgba.r - current.rgba.r;
            int dg = prev.rgba.g - current.rgba.g;
            int db = prev.rgba.b - current.rgba.b;
            int dr_dg = dr-dg;
            int db_dg = db-dg;
            // Check to see if we can store the minor differences
            if (qoi_minor_diff(dr, dg, db)) {
                printf("found minorly different pixel\n");
                NEXTOUT = QOI_OP_DIFF | ((dr+2) << 4) | ((dg+2) << 2) | (db + 2);
                goto nextloop;
            } else if (qoi_luma_diff(dr_dg, dg, db_dg)) {
                printf("found luma different pixel\n");
                NEXTOUT = QOI_OP_LUMA | (dg + 32);
                NEXTOUT = (dr_dg + 8) << 4 | (db_dg + 8);
                goto nextloop;
            }
        }
        
        // Finally, store the whole pixel
        printf("storing whole px\n");
        if (is_rgba) {
            NEXTOUT = QOI_OP_RGBA;
            write_i32(current.v, output, outlen);
        } else {
            write_i32((QOI_OP_RGB << 24) | current.v, output, outlen);
        }

nextloop:
        prev = current;
    }
    
    // write padding data
    for (int i=0; i<(int)sizeof(qoi_padding); i++)
        output[(*outlen)++] = qoi_padding[i];

    *out_status = QOI_OK;
    return output;
    #undef NEXTOUT
}