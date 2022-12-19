#pragma once
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QOI_OP_INDEX = 0,
    QOI_OP_DIFF = 1,
    QOI_OP_LUMA = 2,
    QOI_OP_RUN = 3,
    QOI_OP_RGB = 254,
    QOI_OP_RGBA = 255,
} QOI_CHAR_TYPE;

typedef enum {
    QOI_OK,
    QOI_FILE_ERR,
    QOI_MALLOC_ERR,
    QOI_FREAD_ERR,
    QOI_DECODE_ERR,
} QOI_STATUS;

typedef struct {
    char magic[4];      // "qoif"
    uint32_t width;
    uint32_t height;
    uint8_t channels;   // 3=RGB, 4=RGBA
    uint8_t colorspace; // 0=sRGB with linear alpha, 1=all linear
} qoi_header;

typedef struct {
    qoi_header *header;
    char *pixels;
} qoi_image;


int qoi_read(const char* path, qoi_image *output);
int qoi_write(const char* path, qoi_image *image);
int qoi_encode(const char* data, qoi_image *output);
int qoi_decode(const char* data, char *output);

int qoi_free(qoi_image *image);

#ifdef __cplusplus
}
#endif