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
inline static int qoi_hash_rgba(qoi_rgba px) {
    return px.rgba.r * 3 + px.rgba.g * 5 + px.rgba.b * 7 + px.rgba.a * 11;
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
    memset(lookup, 0, sizeof(lookup));

    // write actual data
    qoi_rgba prev = {.v = 0xFF000000 }; // ABGR
    qoi_rgba current = prev;
    for (int px_idx=0; px_idx < px_len; px_idx += image->header->channels) {
        current.rgba.r = pixels[px_idx+0];       
        current.rgba.g = pixels[px_idx+1];       
        current.rgba.b = pixels[px_idx+2];       
        if (is_rgba)
            current.rgba.a = pixels[px_idx+3];
        
        printf("current: 0x%08X(0x%08X)\t%X %X %X %X\n", current.v, __bswap_32(current.v), current.rgba.r, current.rgba.g, current.rgba.b, current.rgba.a);

        if (current.v == prev.v) {
            printf("found run\n");
            runlen++;
            if (runlen == 62 || px_idx == last_px_idx) {
                NEXTOUT = QOI_OP_RUN | (runlen - 1);
                runlen = 0;
            }
            goto nextloop; // Look, we can't afford all these levels of indentation! In this economy?
        }

        if (runlen > 0) { // no matter what, we can reset the run length
            printf("Ending run\n");
            NEXTOUT = QOI_OP_RUN | (runlen - 1);
            runlen = 0;
        }

        // int index = qoi_hash_rgba(current) % 64;
        // if (lookup[index].v == current.v) {
        //     NEXTOUT = QOI_OP_INDEX | index;
        //     goto nextloop;
        // } 

        // lookup[index] = current;

        
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