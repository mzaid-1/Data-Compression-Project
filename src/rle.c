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
    int consecutive = 0;
    unsigned char last_c = 0;

    while (i < len) {
        unsigned char c = input[i++];
        output[o++] = c;

        if (consecutive > 0 && c == last_c) {
            consecutive++;
        } else {
            consecutive = 1;
            last_c = c;
        }

        if (consecutive == 4) {
            if (i < len) {
                unsigned char extra = input[i++];
                for (int k = 0; k < (int)extra; k++) {
                    output[o++] = c;
                }
            }
            consecutive = 0; 
        }
    }
    *out_len = o;
}

void rle2_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len) {
    size_t i = 0, o = 0;
    while (i < len) {
        if (input[i] != 0) {
            output[o++] = input[i] + 1; /* Offset non-zeros by 2 (wait, RLE-2 standard is non-zeros are val+1) */
            /* Wait, standard RLE-2: non-zeros are val+1, 0s are handled by RUNA/RUNB */
            /* Our implementation: non-zeros map to [2..256] */
            output[o-1] = input[i] + 1; 
            i++;
        } else {
            size_t run = 0;
            while (i < len && input[i] == 0) {
                run++;
                i++;
            }
            /* Bijective base-2: RUNA=0, RUNB=1 */
            while (run > 0) {
                if (run % 2 == 1) {
                    output[o++] = 0; /* RUNA */
                    run = (run - 1) / 2;
                } else {
                    output[o++] = 1; /* RUNB */
                    run = (run - 2) / 2;
                }
            }
        }
    }
    *out_len = o;
}

void rle2_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len) {
    size_t i = 0, o = 0;
    while (i < len) {
        unsigned char c = input[i++];
        if (c >= 2) {
            output[o++] = c - 1;
        } else {
            size_t run = 0;
            size_t p2 = 1;
            /* Read consecutive RUNA/RUNB */
            run += (c + 1) * p2;
            while (i < len && input[i] < 2) {
                p2 <<= 1;
                run += (input[i++] + 1) * p2;
            }
            for (size_t k = 0; k < run; k++) output[o++] = 0;
        }
    }
    *out_len = o;
}