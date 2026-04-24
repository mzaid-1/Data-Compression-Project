#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/bzip2_impl.h"

BlockManager *divide_into_blocks(const char *filename, size_t block_size) {
    if (!filename || block_size == 0) return NULL;

    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[Block] Error: cannot open '%s'\n", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    if (file_size <= 0) {
        fprintf(stderr, "[Block] Error: file is empty.\n");
        fclose(f);
        return NULL;
    }

    int num_blocks = (int)((file_size + block_size - 1) / block_size);

    BlockManager *manager = (BlockManager *)malloc(sizeof(BlockManager));
    if (!manager) { fclose(f); return NULL; }

    manager->blocks     = (Block *)calloc(num_blocks, sizeof(Block));
    manager->num_blocks = num_blocks;
    manager->block_size = block_size;

    if (!manager->blocks) { free(manager); fclose(f); return NULL; }

    long remaining = file_size;
    for (int i = 0; i < num_blocks; i++) {
        size_t this_block = (remaining > (long)block_size) ? block_size : (size_t)remaining;

        manager->blocks[i].data = (unsigned char *)malloc(this_block);
        if (!manager->blocks[i].data) {
            fprintf(stderr, "[Block] malloc failed for block %d\n", i);
            for (int j = 0; j < i; j++) free(manager->blocks[j].data);
            free(manager->blocks);
            free(manager);
            fclose(f);
            return NULL;
        }

        size_t bytes_read = fread(manager->blocks[i].data, 1, this_block, f);
        manager->blocks[i].size          = bytes_read;
        manager->blocks[i].original_size = bytes_read;
        remaining -= (long)bytes_read;
    }

    fclose(f);
    printf("[Block] '%s' -> %d block(s) (block_size=%zu)\n",
           filename, num_blocks, block_size);
    return manager;
}

int reassemble_blocks(BlockManager *manager, const char *output_filename) {
    if (!manager || !output_filename) return -1;

    FILE *f = fopen(output_filename, "wb");
    if (!f) {
        fprintf(stderr, "[Block] Cannot create '%s'\n", output_filename);
        return -1;
    }

    for (int i = 0; i < manager->num_blocks; i++) {
        if (!manager->blocks[i].data || manager->blocks[i].size == 0) continue;
        size_t w = fwrite(manager->blocks[i].data, 1, manager->blocks[i].size, f);
        if (w != manager->blocks[i].size) {
            fprintf(stderr, "[Block] Short write on block %d\n", i);
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    printf("[Block] Reassembled %d block(s) -> '%s'\n",
           manager->num_blocks, output_filename);
    return 0;
}

void free_block_manager(BlockManager *manager) {
    if (!manager) return;
    if (manager->blocks) {
        for (int i = 0; i < manager->num_blocks; i++)
            free(manager->blocks[i].data);
        free(manager->blocks);
    }
    free(manager);
}