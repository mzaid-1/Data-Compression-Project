
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/bzip2_impl.h"

int main() {
    size_t n = 10000;
    unsigned char *in = malloc(n);
    memset(in, 'A', n);

    size_t out_len = 0;
    unsigned char *compressed = malloc(n * 2 + 4096);
    
    printf("Testing 10,000 'A's...\n");
    
    /* Simulate pipeline */
    Config cfg = { .block_size = 900000, .rle1_enabled = 1, .mtf_enabled = 1, .rle2_enabled = 1, .huffman_enabled = 1 };
    
    /* We'll use a temp file to use the existing do_compress logic or just call stages */
    FILE *f = fopen("test_in.tmp", "wb");
    fwrite(in, 1, n, f);
    fclose(f);
    
    extern int do_compress(const char *infile, const char *outfile, Config *cfg);
    extern int do_decompress(const char *infile, const char *outfile, Config *cfg);
    
    do_compress("test_in.tmp", "test_out.bz2i", &cfg);
    do_decompress("test_out.bz2i", "test_rec.tmp", &cfg);
    
    FILE *f2 = fopen("test_rec.tmp", "rb");
    unsigned char *rec = malloc(n);
    fread(rec, 1, n, f2);
    fclose(f2);
    
    if (memcmp(in, rec, n) == 0) {
        printf("SUCCESS\n");
    } else {
        printf("FAILURE\n");
        for(int i=0; i<10; i++) printf("%02X ", rec[i]);
        printf("...\n");
    }
    
    return 0;
}
