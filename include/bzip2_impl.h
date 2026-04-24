#ifndef BZIP2_IMPL_H
#define BZIP2_IMPL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    unsigned char *data;
    size_t size;
    size_t original_size;
} Block;

typedef struct {
    Block  *blocks;
    int     num_blocks;
    size_t  block_size;
} BlockManager;

typedef struct {
    int index;
} Rotation;

typedef struct {
    size_t block_size;
    int    rle1_enabled;
    char   bwt_type[32];
    int    output_metrics;
    char   input_directory[256];
    char   output_directory[256];
} Config;

Config load_config(const char *config_file);
void   print_config(const Config *cfg);

BlockManager *divide_into_blocks(const char *filename, size_t block_size);
int           reassemble_blocks(BlockManager *manager, const char *output_filename);
void          free_block_manager(BlockManager *manager);

void rle1_encode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len);

void rle1_decode(unsigned char *input,  size_t len,
                 unsigned char *output, size_t *out_len);

int  compare_rotations(const void *a, const void *b);

void bwt_encode(unsigned char *input,  size_t len,
                unsigned char *output, int *primary_index);

void bwt_decode(unsigned char *input,  size_t len,
                int primary_index,     unsigned char *output);

double get_time_ms(void);

#endif