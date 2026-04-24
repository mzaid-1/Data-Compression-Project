#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/bzip2_impl.h"

#define MAGIC     "BZ2PH1\0\0"
#define MAGIC_LEN 8

#define BUF_SIZE(n) ((n) * 2 + 1024)

static void write_u32(FILE *f, uint32_t v) {
    unsigned char b[4] = {
        (unsigned char)( v        & 0xFF),
        (unsigned char)((v >>  8) & 0xFF),
        (unsigned char)((v >> 16) & 0xFF),
        (unsigned char)((v >> 24) & 0xFF)
    };
    fwrite(b, 1, 4, f);
}

static uint32_t read_u32(FILE *f) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0xFFFFFFFF;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static int do_compress(const char *infile, const char *outfile, Config *cfg) {
    printf("\n=== COMPRESS: %s -> %s ===\n", infile, outfile);
    double t0 = get_time_ms();

    BlockManager *bm = divide_into_blocks(infile, cfg->block_size);
    if (!bm) return 1;

    FILE *out = fopen(outfile, "wb");
    if (!out) {
        fprintf(stderr, "Cannot create output file '%s'\n", outfile);
        free_block_manager(bm);
        return 1;
    }

    fwrite(MAGIC, 1, MAGIC_LEN, out);

    long total_in = 0, total_out = MAGIC_LEN;

    for (int i = 0; i < bm->num_blocks; i++) {
        unsigned char *blk  = bm->blocks[i].data;
        size_t         blen = bm->blocks[i].size;
        total_in += (long)blen;

        size_t bufsz = BUF_SIZE(blen);
        unsigned char *tmp1 = (unsigned char *)malloc(bufsz);
        unsigned char *tmp2 = (unsigned char *)malloc(bufsz);
        if (!tmp1 || !tmp2) {
            fprintf(stderr, "malloc failed\n");
            free(tmp1); free(tmp2);
            fclose(out); free_block_manager(bm);
            return 1;
        }

        unsigned char *cur    = blk;
        size_t         curlen = blen;
        int            pidx   = 0;

        if (cfg->rle1_enabled) {
            size_t rle_len = 0;
            rle1_encode(cur, curlen, tmp1, &rle_len);
            memcpy(tmp2, tmp1, rle_len);
            cur    = tmp2;
            curlen = rle_len;
        }

        bwt_encode(cur, curlen, tmp1, &pidx);

        write_u32(out, (uint32_t)blen);
        write_u32(out, (uint32_t)curlen);
        write_u32(out, (uint32_t)pidx);
        fwrite(tmp1, 1, curlen, out);
        total_out += 4 + 4 + 4 + (long)curlen;

        if (cfg->output_metrics)
            printf("  Block %d: %zu bytes  (rle1=%zu, bwt=%zu, pidx=%d)\n",
                   i, blen, curlen, curlen, pidx);

        free(tmp1);
        free(tmp2);
    }

    write_u32(out, 0xFFFFFFFF);
    total_out += 4;
    fclose(out);
    free_block_manager(bm);

    double elapsed = get_time_ms() - t0;
    printf("  Done: %ld -> %ld bytes  (%.1f%%)  %.1f ms\n",
           total_in, total_out, 100.0 * total_out / total_in, elapsed);
    return 0;
}

static int do_decompress(const char *infile, const char *outfile, Config *cfg) {
    printf("\n=== DECOMPRESS: %s -> %s ===\n", infile, outfile);
    double t0 = get_time_ms();

    FILE *in = fopen(infile, "rb");
    if (!in) { fprintf(stderr, "Cannot open '%s'\n", infile); return 1; }

    char mbuf[MAGIC_LEN];
    if (fread(mbuf, 1, MAGIC_LEN, in) != MAGIC_LEN ||
        memcmp(mbuf, MAGIC, MAGIC_LEN) != 0) {
        fprintf(stderr, "Not a valid Phase 1 compressed file.\n");
        fclose(in);
        return 1;
    }

    FILE *out = fopen(outfile, "wb");
    if (!out) { fprintf(stderr, "Cannot create '%s'\n", outfile); fclose(in); return 1; }

    int block_num = 0;
    while (1) {
        uint32_t orig_sz  = read_u32(in);
        if (orig_sz == 0xFFFFFFFF) break;
        uint32_t enc_sz   = read_u32(in);
        uint32_t pidx     = read_u32(in);

        unsigned char *enc = (unsigned char *)malloc(enc_sz);
        if (!enc) { fprintf(stderr, "malloc failed\n"); break; }

        if (fread(enc, 1, enc_sz, in) != enc_sz) {
            fprintf(stderr, "Short read on block %d\n", block_num);
            free(enc); break;
        }

        size_t bufsz = BUF_SIZE(orig_sz);
        unsigned char *tmp1 = (unsigned char *)malloc(bufsz);
        unsigned char *tmp2 = (unsigned char *)malloc(bufsz);
        if (!tmp1 || !tmp2) { free(enc); free(tmp1); free(tmp2); break; }

        bwt_decode(enc, enc_sz, (int)pidx, tmp1);

        if (cfg->rle1_enabled) {
            size_t dec_len = 0;
            rle1_decode(tmp1, enc_sz, tmp2, &dec_len);
            fwrite(tmp2, 1, dec_len, out);
            if (cfg->output_metrics)
                printf("  Block %d: %u -> %zu bytes\n", block_num, enc_sz, dec_len);
        } else {
            fwrite(tmp1, 1, enc_sz, out);
        }

        free(enc); free(tmp1); free(tmp2);
        block_num++;
    }

    fclose(in);
    fclose(out);

    double elapsed = get_time_ms() - t0;
    printf("  Decoded %d block(s) in %.1f ms\n", block_num, elapsed);
    return 0;
}

static int run_tests(void) {
    int pass = 0, fail = 0;

    printf("\n==============================\n");
    printf("  PHASE 1 — UNIT TESTS\n");
    printf("==============================\n\n");

#define CHECK(name, cond) do { \
    if (cond) { printf("  [PASS] %s\n", (name)); pass++; } \
    else      { printf("  [FAIL] %s\n", (name)); fail++; } \
} while(0)

    {
        unsigned char in[14];
        memset(in, 'A', 10);
        memcpy(in + 10, "BCDE", 4);
        unsigned char enc[64], dec[64];
        size_t enc_len = 0, dec_len = 0;
        rle1_encode(in, 14, enc, &enc_len);
        rle1_decode(enc, enc_len, dec, &dec_len);
        CHECK("RLE-1 long run round-trip",    dec_len == 14 && memcmp(in, dec, 14) == 0);
        CHECK("RLE-1 long run compresses",    enc_len < 14);
    }

    {
        unsigned char in[] = "ABCDEFGH";
        size_t n = 8;
        unsigned char enc[64], dec[64];
        size_t enc_len = 0, dec_len = 0;
        rle1_encode(in, n, enc, &enc_len);
        rle1_decode(enc, enc_len, dec, &dec_len);
        CHECK("RLE-1 no-run round-trip",      dec_len == n && memcmp(in, dec, n) == 0);
    }

    {
        unsigned char in[100]; memset(in, 'Z', 100);
        unsigned char enc[64], dec[256];
        size_t enc_len = 0, dec_len = 0;
        rle1_encode(in, 100, enc, &enc_len);
        rle1_decode(enc, enc_len, dec, &dec_len);
        CHECK("RLE-1 all-same round-trip",    dec_len == 100 && memcmp(in, dec, 100) == 0);
        CHECK("RLE-1 all-same compresses",    enc_len < 100);
    }

    {
        unsigned char in[259]; memset(in, 'X', 259);
        unsigned char enc[64], dec[512];
        size_t enc_len = 0, dec_len = 0;
        rle1_encode(in, 259, enc, &enc_len);
        rle1_decode(enc, enc_len, dec, &dec_len);
        CHECK("RLE-1 max-run (259) round-trip", dec_len == 259 && memcmp(in, dec, 259) == 0);
        CHECK("RLE-1 max-run encodes to 5 bytes", enc_len == 5);
    }

    {
        unsigned char in[] = "banana";
        size_t n = 6;
        unsigned char bwt_out[16], bwt_inv[16];
        int pidx = -1;
        bwt_encode(in, n, bwt_out, &pidx);
        CHECK("BWT 'banana' output = 'nnbaaa'", memcmp(bwt_out, "nnbaaa", 6) == 0);
        CHECK("BWT 'banana' primary_index = 3",  pidx == 3);
        bwt_decode(bwt_out, n, pidx, bwt_inv);
        CHECK("BWT 'banana' round-trip",         memcmp(in, bwt_inv, n) == 0);
    }

    {
        unsigned char in[] = "abracadabra";
        size_t n = 11;
        unsigned char enc[32], dec[32];
        int pidx = -1;
        bwt_encode(in, n, enc, &pidx);
        bwt_decode(enc, n, pidx, dec);
        CHECK("BWT 'abracadabra' round-trip",    memcmp(in, dec, n) == 0);
        CHECK("BWT primary_index in range",       pidx >= 0 && pidx < (int)n);
    }

    {
        unsigned char in[] = "mississippi";
        size_t n = 11;
        unsigned char enc[32], dec[32];
        int pidx = -1;
        bwt_encode(in, n, enc, &pidx);
        bwt_decode(enc, n, pidx, dec);
        CHECK("BWT 'mississippi' round-trip",    memcmp(in, dec, n) == 0);
    }

    {
        unsigned char in[] = "the quick brown fox jumps over the lazy dog "
                             "the quick brown fox jumps over the lazy dog";
        size_t n = strlen((char *)in);
        size_t bufsz = n * 2 + 256;
        unsigned char *b1 = (unsigned char *)malloc(bufsz);
        unsigned char *b2 = (unsigned char *)malloc(bufsz);
        unsigned char *b3 = (unsigned char *)malloc(bufsz);
        unsigned char *b4 = (unsigned char *)malloc(bufsz);
        size_t rle_enc_len = 0, rle_dec_len = 0;
        int pidx = -1;

        rle1_encode(in, n, b1, &rle_enc_len);
        bwt_encode(b1, rle_enc_len, b2, &pidx);
        bwt_decode(b2, rle_enc_len, pidx, b3);
        rle1_decode(b3, rle_enc_len, b4, &rle_dec_len);

        CHECK("Phase 1 pipeline round-trip length",  rle_dec_len == n);
        CHECK("Phase 1 pipeline round-trip content", memcmp(in, b4, n) == 0);

        free(b1); free(b2); free(b3); free(b4);
    }

    printf("\n==============================\n");
    printf("  %d passed,  %d failed\n", pass, fail);
    printf("==============================\n\n");
    return (fail == 0) ? 0 : 1;
}

static void usage(const char *prog) {
    printf("Usage:\n");
    printf("  %s compress   <input>      <output.bz2p1>  [config.ini]\n", prog);
    printf("  %s decompress <input.bz2p1> <output>       [config.ini]\n", prog);
    printf("  %s test\n", prog);
    printf("  %s info       <input.bz2p1>\n", prog);
}

static int do_info(const char *infile) {
    FILE *f = fopen(infile, "rb");
    if (!f) { fprintf(stderr, "Cannot open '%s'\n", infile); return 1; }

    char mbuf[MAGIC_LEN];
    if (fread(mbuf, 1, MAGIC_LEN, f) != MAGIC_LEN || memcmp(mbuf, MAGIC, MAGIC_LEN) != 0) {
        printf("Not a Phase 1 compressed file.\n"); fclose(f); return 1;
    }

    printf("File: %s  [BZ2PH1 format]\n", infile);
    int blk = 0; uint32_t to = 0, te = 0;
    while (1) {
        uint32_t orig = read_u32(f); if (orig == 0xFFFFFFFF) break;
        uint32_t enc  = read_u32(f);
        uint32_t pidx = read_u32(f);
        fseek(f, enc, SEEK_CUR);
        printf("  Block %d: orig=%u  enc=%u  ratio=%.1f%%  pidx=%u\n",
               blk, orig, enc, 100.0f * enc / orig, pidx);
        to += orig; te += enc; blk++;
    }
    printf("Total: orig=%u  enc=%u  ratio=%.1f%%  blocks=%d\n",
           to, te, 100.0f * te / to, blk);
    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    const char *cmd = argv[1];

    if (strcmp(cmd, "test") == 0)
        return run_tests();

    if (strcmp(cmd, "info") == 0) {
        if (argc < 3) { usage(argv[0]); return 1; }
        return do_info(argv[2]);
    }

    if (argc < 4) { usage(argv[0]); return 1; }

    const char *cfg_path = (argc >= 5) ? argv[4] : "config.ini";
    Config cfg = load_config(cfg_path);
    print_config(&cfg);

    if      (strcmp(cmd, "compress")   == 0) return do_compress(argv[2], argv[3], &cfg);
    else if (strcmp(cmd, "decompress") == 0) return do_decompress(argv[2], argv[3], &cfg);
    else { fprintf(stderr, "Unknown command: %s\n", cmd); usage(argv[0]); return 1; }
}