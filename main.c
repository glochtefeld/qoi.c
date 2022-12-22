#include "qoi.h"
#include <stdlib.h>
#include <stdio.h>

int main() {
    qoi_header testhd = {.width=4, .height=1, .channels=3, .colorspace=0};
    char testpx[] = { 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x01, 0x02, 0x02 };
    qoi_image test;
    test.header = &testhd;
    test.pixels = testpx;

    uint32_t data_len;
    qoi_status st;
    uint8_t *data = qoi_encode(&test, &data_len, &st);

    if (st != QOI_OK) {
        printf("Encode recieved status of %s\n", GET_QOI_STATUS(st));
        return 1;
    }
    printf("Length: %d;\n", data_len);
    for (int i = 0; i < data_len; i++) {
        printf("Byte %3d:\t%X\n", i, data[i]);
    }
    free(data);
    return 0;
}