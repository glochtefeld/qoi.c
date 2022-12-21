#include "qoi.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define QOI_MAGIC ('q'<<24 | 'o' << 16 | 'i' << 8 | 'f')

static uint32_t switch_endian(uint32_t n) {
    return ((n >> 24)   & 0x000000FF)   // 3 -> 0
        | ((n << 8)     & 0x00FF0000)   // 1 -> 2
        | ((n >> 8)     & 0x0000FF00)   // 2 -> 1
        | ((n << 24)    & 0xFF000000);  // 0 -> 3
}

qoi_status qoi_read(const char *path, qoi_image *output) {
    // 1. read data
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return QOI_BAD_FPATH;
    }
    fseek(f, 0L, SEEK_END);
    size_t filesize = ftell(f);
    rewind(f);
    char *buffer = malloc(filesize + 1);
    if (buffer == NULL) {
        return QOI_CANNOT_MALLOC;
    }
    size_t bytesRead = fread(buffer, sizeof(char), filesize, f);
    if (bytesRead < filesize) {
        return QOI_FREAD_ERR;
    }
    buffer[bytesRead] = '\0';
    fclose(f);

    // 2. Copy all that data into the output
    int decoded = qoi_decode(buffer, filesize + 1, output->pixels);
    if (decoded != 0) {
        return QOI_DECODE_ERR;
    }

    return QOI_OK;
}

int qoi_free(qoi_image *image) {
    free(image->pixels);
    free(image->header);
}

static const uint8_t qoi_padding[8] = {0,0,0,0,0,0,0,1};
static uint32_t read_int (unsigned char *bytes, int *p) {
    int a = bytes[(*p)++] << 24;
    int r = bytes[(*p)++] << 16;
    int g = bytes[(*p)++] << 8;
    int b = bytes[(*p)++];
    return a | r | g | b;
}

// Purpose: encode an array of RGBA values into a valid QOI format
// TODO: make it so this works with both rgb and rgba instead of just rgba
qoi_status qoi_encode(qoi_header *header, const char* data, uint32_t length, qoi_image *output) {
    if (header == NULL || data == NULL || output == NULL)
        return QOI_NULLPTR_PASSED;
    if (header->width == 0 || header->height == 0
        || header->channels < 3 || header->channels > 4
        || header->colorspace > 1) {
            return QOI_BAD_HEADER;
        }
    // 1. Set header data
    output->header = &header;

    // 2. Create index lookup array
    qoi_rgba indices[64];
    memset(indices, 0, 64);

    // 3. Allocate memory for the data output
    qoi_rgba *data_out = malloc(sizeof(qoi_rgba) * length + sizeof(qoi_padding));
    if (data_out == NULL)
        return QOI_CANNOT_MALLOC;

    // 4. loop over input data, checking for possible compressions
    int out_idx = 0;
    qoi_rgba prev, current, next;
    prev.v=0xFF0000000; // equivalent to r=0,b=0,g=0,a=255
    current.v = read_int(data, 0);
    next.v = read_int(data, header->channels);
    for (int i=0; i < length; next.v = read_int(data, i)) {
        int currenthash = qoi_hash(data[i], 1);
        if (current.v == prev.v && current.v == next.v) { // Run detected
        }
        else if (current.v == prev.v) { // Not a run but a duplicate

        }

        // next iter prep
        prev.v = current.v;
        current.v = next.v;
        i += header->channels;
    }
    output->pixels = &data_out;
}

// Purpose: Decode a datastream into image data
qoi_status qoi_decode(const char* data, uint32_t length, char *output) {

}

static int qoi_hash(char *pixel, int use_alpha) { // rgba
    return (pixel[0] * 3
        + pixel[1] * 5
        + pixel[2] * 7
        + ( use_alpha ? pixel[3] : 255 ) * 11) % 64;
}
