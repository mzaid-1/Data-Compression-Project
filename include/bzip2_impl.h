#ifndef BZIP2_IMPL_H
#define BZIP2_IMPL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ============================================================
 * BLOCK MANAGEMENT STRUCTURES
 * ============================================================ */

typedef struct {
    unsigned char *data;        /* Pointer to block data */
    size_t size;                /* Current size of block */
    size_t original_size;       /* Original size before compression */
} Block;

typedef struct {
    Block *blocks;              /* Array of blocks */
    int num_blocks;             /* Number of blocks */
    size_t block_size;          /* Configurable block size */
} BlockManager;

/* ============================================================
 * BWT STRUCTURES
 * ============================================================ */

typedef struct {
    int index;                  /* Original index (store index, not full string) */
} Rotation;

/* ============================================================
 * HUFFMAN STRUCTURES
 * ============================================================ */

typedef struct {
    uint32_t code;              /* Huffman code (32-bit to prevent overflow) */
    unsigned char length;       /* Code length in bits */
} HuffmanCode;

typedef struct Node {
    unsigned char symbol;       /* Byte value (0-255) */
    int freq;                   /* Frequency count */
    struct Node *left;          /* Left child */
    struct Node *right;         /* Right child */
} HuffmanNode;

/* ============================================================
 * CONFIGURATION STRUCTURE
 * ============================================================ */

typedef struct {
    size_t block_size;
    int rle1_enabled;
    int mtf_enabled;
    int rle2_enabled;
    int huffman_enabled;
    int benchmark_mode;
    int output_metrics;
    char bwt_type[32];
    char input_directory[256];
    char output_directory[256];
} Config;

/* ============================================================
 * FUNCTION PROTOTYPES - CONFIG
 * ============================================================ */

Config load_config(const char *config_file);
void print_config(const Config *cfg);

/* ============================================================
 * FUNCTION PROTOTYPES - BLOCK MANAGEMENT
 * ============================================================ */

BlockManager *divide_into_blocks(const char *filename, size_t block_size);
int reassemble_blocks(BlockManager *manager, const char *output_filename);
void free_block_manager(BlockManager *manager);

/* ============================================================
 * FUNCTION PROTOTYPES - RLE-1
 * ============================================================ */

void rle1_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);

void rle1_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);

/* ============================================================
 * FUNCTION PROTOTYPES - BWT
 * ============================================================ */

void bwt_encode(unsigned char *input, size_t len,
                unsigned char *output, int *primary_index);

void bwt_decode(unsigned char *input, size_t len,
                int primary_index, unsigned char *output);

/* ============================================================
 * FUNCTION PROTOTYPES - MTF
 * ============================================================ */

void mtf_encode(unsigned char *input, size_t len, unsigned char *output);
void mtf_decode(unsigned char *input, size_t len, unsigned char *output);

/* ============================================================
 * FUNCTION PROTOTYPES - RLE-2
 * ============================================================ */

void rle2_encode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);

void rle2_decode(unsigned char *input, size_t len,
                 unsigned char *output, size_t *out_len);

/* ============================================================
 * FUNCTION PROTOTYPES - HUFFMAN
 * ============================================================ */

void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len);
void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len);
void free_huffman_tree(HuffmanNode *node);

/* ============================================================
 * FUNCTION PROTOTYPES - UTILITIES
 * ============================================================ */

void print_hex(const char *label, unsigned char *data, size_t len);
double get_time_ms(void);

#endif /* BZIP2_IMPL_H */