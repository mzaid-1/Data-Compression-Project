#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/bzip2_impl.h"

static unsigned char *g_data;
static size_t         g_len;

int compare_rotations(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    size_t n = g_len;

    for (size_t k = 0; k < n; k++) {
        unsigned char ca = g_data[(ia + k) % n];
        unsigned char cb = g_data[(ib + k) % n];
        if (ca != cb) return (ca < cb) ? -1 : 1;
    }
    return 0;
}

void bwt_encode(unsigned char *input, size_t len,
                unsigned char *output, int *primary_index) {
    if (!input || len == 0 || !output || !primary_index) return;

    int *idx = (int *)malloc(len * sizeof(int));
    if (!idx) { fprintf(stderr, "[BWT] malloc failed\n"); return; }

    for (size_t i = 0; i < len; i++) idx[i] = (int)i;

    g_data = input;
    g_len  = len;
    qsort(idx, len, sizeof(int), compare_rotations);

    *primary_index = -1;
    for (size_t i = 0; i < len; i++) {
        output[i] = input[(idx[i] + len - 1) % len];
        if (idx[i] == 0) *primary_index = (int)i;
    }

    free(idx);
}

void bwt_decode(unsigned char *input, size_t len,
                int primary_index, unsigned char *output) {
    if (!input || len == 0 || !output || primary_index < 0) return;

    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[(unsigned char)input[i]]++;

    int start[256] = {0};
    int total = 0;
    for (int c = 0; c < 256; c++) {
        start[c] = total;
        total += freq[c];
    }

    int *lf = (int *)malloc(len * sizeof(int));
    if (!lf) { fprintf(stderr, "[BWT] malloc failed\n"); return; }

    int cnt[256] = {0};
    for (size_t i = 0; i < len; i++) {
        unsigned char c = input[i];
        lf[i] = start[c] + cnt[c];
        cnt[c]++;
    }

    int pos = primary_index;
    for (int i = (int)len - 1; i >= 0; i--) {
        output[i] = input[pos];
        pos = lf[pos];
    }

    free(lf);
}