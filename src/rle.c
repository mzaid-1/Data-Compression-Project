#include <stdio.h>
#include <stdlib.h>
#include "../include/bzip2_impl.h"

void rle1_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len) {
    size_t i = 0, o = 0;

    while (i < len) {
        unsigned char c = input[i];
        size_t run = 1;

        while (run < 259 && i + run < len && input[i + run] == c)
            run++;

        if (run >= 4) {

            output[o++] = c;
            output[o++] = c;
            output[o++] = c;
            output[o++] = c;
            output[o++] = (unsigned char)(run - 4);
            i += run;
        } else {

            output[o++] = c;
            i++;
        }
    }

    *out_len = o;
}

void rle1_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len) {
    size_t i = 0, o = 0;

    while (i < len) {
        unsigned char c = input[i++];
        output[o++] = c;

        if (i + 2 < len &&
            input[i] == c && input[i+1] == c && input[i+2] == c) {

            output[o++] = c;
            output[o++] = c;
            output[o++] = c;
            i += 3;

            if (i < len) {
                unsigned char extra = input[i++];
                for (unsigned char k = 0; k < extra; k++)
                    output[o++] = c;
            }
        }
    }

    *out_len = o;
}