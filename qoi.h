#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { // Note: QOI spec is big endian
    QOI_OP_INDEX = 0x00000000, // first byte is 0
    QOI_OP_DIFF = 0x00000001, // first byte is 1
    QOI_OP_LUMA = 0x00000002, // first byte is 2
    QOI_OP_RUN = 0x00000003, // first byte is 3
    QOI_OP_RGB = 0x000000FE, // first byte is 254
    QOI_OP_RGBA = 0x000000FF, // first byte is 255
} QOI_BIT_MASK;

typedef enum {
    QOI_OK,
    QOI_BAD_FPATH,
    QOI_CANNOT_MALLOC,
    QOI_FREAD_ERR,
    QOI_DECODE_ERR,
    QOI_NULLPTR_PASSED,
    QOI_BAD_HEADER,
} qoi_status;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t channels;   // 3=RGB, 4=RGBA
    uint8_t colorspace; // 0=sRGB with linear alpha, 1=all linear
} qoi_header;

typedef union {
    struct { unsigned char r, g, b, a; } rgba;
    unsigned int v;
} qoi_rgba;

typedef struct {
    qoi_header *header;
    qoi_rgba *pixels;
} qoi_image;


qoi_status qoi_read(const char* path, qoi_image *output);
qoi_status qoi_write(const char* path, qoi_image *image);
qoi_status qoi_encode(qoi_header *header, const char* data, uint32_t length, qoi_image *output);
qoi_status qoi_decode(const char* data, uint32_t length, char *output);

int qoi_free(qoi_image *image);

#ifdef __cplusplus
}
#endif