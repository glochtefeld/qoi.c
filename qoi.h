#ifndef QOI_H
#define QOI_H
#include <stdint.h>

#define QOI_OP_INDEX	0x00 // 00xxxxxx
#define QOI_OP_DIFF	    0x40 // 01xxxxxx
#define QOI_OP_LUMA	    0x80 // 10xxxxxx
#define QOI_OP_RUN	    0xC0 // 11xxxxxx
#define QOI_OP_RGB	    0xFE // 11111110
#define QOI_OP_RGBA	    0xFF // 11111111

#define QOI_HDR_LEN    14
#define QOI_PADDING_LEN 8
#define QOI_MAGIC ('q' << 24 | 'o' << 16 | 'i' << 8 | 'f' )

typedef enum {
    QOI_OK,
    QOI_BAD_FPATH,
    QOI_CANNOT_MALLOC,
    QOI_FREAD_ERR,
    QOI_DECODE_ERR,
    QOI_NULLPTR_PASSED,
    QOI_BAD_HEADER,
} qoi_status;

extern const char *statuses[];

#define GET_QOI_STATUS(i) statuses[i]

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t channels;   // 3=RGB, 4=RGBA
    uint8_t colorspace; // 0=sRGB with linear alpha, 1=all linear
} qoi_header;

typedef union {
    struct { uint8_t r, g, b, a; } rgba;
    uint32_t v;
} qoi_rgba;
#define QOI_HASH(C) (C.rgba.r * 3 + C.rgba.g * 5 + C.rgba.b * 7 + C.rgba.a * 11)

typedef struct {
    qoi_header *header;
    char *pixels;
} qoi_image;


// Given a filepath, will read in a qoi image and decode it into the header and raw bytes.
qoi_status qoi_read(const char* path, qoi_image *output);
// Given an array of bytes and its length, returns the header and bytes for the file. Driver for qoi_read().
qoi_status qoi_decode(const char* data, uint32_t length, qoi_image *output);

// Given an image, encodes and writes that image to path.
qoi_status qoi_write(const char* path, const qoi_image *image);
// Given an image, returns the encoded image data. 
uint8_t *qoi_encode(const qoi_image *image, uint32_t *outlen, qoi_status *out_status);

#endif // QOI_H