#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/bzip2_impl.h"

/* ============================================================
 * MOVE-TO-FRONT TRANSFORM
 *
 * Maintains a list (alphabet) of all 256 byte values.
 * Initially: list = [0, 1, 2, ..., 255]
 *
 * Encoding:
 *   For each input byte c:
 *     1. Find position (index) of c in the list.
 *     2. Emit the index.
 *     3. Move c to the front of the list.
 *
 * Decoding:
 *   For each input index i:
 *     1. Emit list[i].
 *     2. Move list[i] to the front.
 *
 * After BWT, the same byte values cluster together, so most
 * indices will be 0 or small — this helps RLE-2 greatly.
 * ============================================================ */

void mtf_encode(unsigned char *input, size_t len, unsigned char *output) {
    if (!input || !output || len == 0) return;

    /* Initialise list to [0, 1, 2, ..., 255] */
    unsigned char list[256];
    for (int i = 0; i < 256; i++) list[i] = (unsigned char)i;

    for (size_t i = 0; i < len; i++) {
        unsigned char c = input[i];

        /* Find position of c in list */
        int pos = 0;
        while (pos < 256 && list[pos] != c) pos++;

        output[i] = (unsigned char)pos;

        /* Move c to front: shift list[0..pos-1] right by 1 */
        if (pos > 0) {
            memmove(list + 1, list, (size_t)pos);
            list[0] = c;
        }
    }
}

void mtf_decode(unsigned char *input, size_t len, unsigned char *output) {
    if (!input || !output || len == 0) return;

    /* Initialise list to [0, 1, 2, ..., 255] */
    unsigned char list[256];
    for (int i = 0; i < 256; i++) list[i] = (unsigned char)i;

    for (size_t i = 0; i < len; i++) {
        int pos = (int)(unsigned char)input[i];
        unsigned char c = list[pos];
        output[i] = c;

        /* Move c to front */
        if (pos > 0) {
            memmove(list + 1, list, (size_t)pos);
            list[0] = c;
        }
    }
}
