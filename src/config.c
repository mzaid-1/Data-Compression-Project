#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/bzip2_impl.h"

static void trim(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) *end-- = '\0';
}

static void strip_comment(char *s) {
    char *p = strchr(s, '#');
    if (p) *p = '\0';
}

Config load_config(const char *config_file) {

    Config cfg;
    cfg.block_size    = 500000;
    cfg.rle1_enabled  = 1;
    cfg.output_metrics = 1;
    strncpy(cfg.bwt_type,         "matrix",       sizeof(cfg.bwt_type) - 1);
    strncpy(cfg.input_directory,  "./benchmarks/", sizeof(cfg.input_directory) - 1);
    strncpy(cfg.output_directory, "./results/",    sizeof(cfg.output_directory) - 1);

    FILE *f = fopen(config_file, "r");
    if (!f) {
        fprintf(stderr, "[Config] '%s' not found — using defaults.\n", config_file);
        return cfg;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        strip_comment(line);
        trim(line);
        if (line[0] == '\0' || line[0] == '[') continue;

        char key[128] = {0}, val[256] = {0};
        if (sscanf(line, "%127[^=]=%255[^\n]", key, val) != 2) continue;
        trim(key); trim(val);

        if      (strcmp(key, "block_size")      == 0) cfg.block_size      = (size_t)atol(val);
        else if (strcmp(key, "rle1_enabled")    == 0) cfg.rle1_enabled    = (strcmp(val,"true")==0);
        else if (strcmp(key, "output_metrics")  == 0) cfg.output_metrics  = (strcmp(val,"true")==0);
        else if (strcmp(key, "bwt_type")        == 0) snprintf(cfg.bwt_type, sizeof(cfg.bwt_type), "%s", val);
        else if (strcmp(key, "input_directory") == 0) snprintf(cfg.input_directory, sizeof(cfg.input_directory), "%s", val);
        else if (strcmp(key, "output_directory")== 0) snprintf(cfg.output_directory, sizeof(cfg.output_directory), "%s", val);
    }
    fclose(f);
    return cfg;
}

void print_config(const Config *cfg) {
    printf("=== Configuration ===\n");
    printf("  block_size     : %zu bytes\n", cfg->block_size);
    printf("  rle1_enabled   : %s\n",  cfg->rle1_enabled  ? "true" : "false");
    printf("  bwt_type       : %s\n",  cfg->bwt_type);
    printf("  output_metrics : %s\n",  cfg->output_metrics ? "true" : "false");
    printf("  input_dir      : %s\n",  cfg->input_directory);
    printf("  output_dir     : %s\n",  cfg->output_directory);
    printf("=====================\n");
}