#include "qoi.h"
#include "stdlib.h"
#include "string.h"

int qoi_read(const char *path, qoi_image *output) {
    // 1. read data
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        printf("could not open %s for reading\n", path);
        return 1;
    }
    fseek(f, 0L, SEEK_END);
    size_t filesize = ftell(f);
    rewind(f);
    char *buffer = malloc(filesize + 1);
    if (buffer == NULL) {
        printf("Could not allocate memory for image buffer %s\n", path);
        return 2;
    }
    size_t bytesRead = fread(buffer, sizeof(char), filesize, f);
    if (bytesRead < filesize) {
        printf("Could not copy %s into buffer\n", path);
        return 3;
    }
    buffer[bytesRead] = '\0';

    // 2. Copy all that data into the output
    int decoded = qoi_decode(buffer, output->pixels);
    if (decoded != 0) {
        printf("Could not decode %s\n", path);
        return 4;
    }

    return 0; // OK
}

int qoi_free(qoi_image *image) {
    free(image->pixels);
    free(image->header);
}

static char previously_seen[64];

static int qoi_hash(char pixel[4]) {
    return (pixel[0] * 3
        + pixel[1] * 5
        + pixel[2] * 7
        + pixel[3] * 11) % 64;
}
