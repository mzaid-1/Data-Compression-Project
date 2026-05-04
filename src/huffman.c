#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/bzip2_impl.h"

/* ============================================================
 * CANONICAL HUFFMAN CODING
 *
 * Steps:
 *   1. Count frequencies.
 *   2. Build a min-heap-based Huffman tree.
 *   3. Assign code lengths by traversing the tree.
 *   4. Canonicalize: sort symbols by (length, symbol value),
 *      assign codes starting from 0 in each length group.
 *   5. Encode: emit canonical codes bit by bit.
 *   6. Header: store 256 code lengths (1 byte each) so the
 *      decoder can reconstruct the canonical codebook.
 *
 * ============================================================ */

/* -------- Min-heap (priority queue) for building tree -------- */

#define MAX_HEAP 512   /* 256 leaves + 255 internal nodes + margin */

typedef struct {
    HuffmanNode *nodes[MAX_HEAP];
    int size;
} MinHeap;

static void heap_push(MinHeap *h, HuffmanNode *n) {
    int i = h->size++;
    h->nodes[i] = n;
    /* Bubble up */
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->nodes[parent]->freq <= h->nodes[i]->freq) break;
        HuffmanNode *tmp = h->nodes[parent];
        h->nodes[parent] = h->nodes[i];
        h->nodes[i] = tmp;
        i = parent;
    }
}

static HuffmanNode *heap_pop(MinHeap *h) {
    if (h->size == 0) return NULL;
    HuffmanNode *top = h->nodes[0];
    h->nodes[0] = h->nodes[--h->size];
    /* Bubble down */
    int i = 0;
    while (1) {
        int l = 2*i+1, r = 2*i+2, smallest = i;
        if (l < h->size && h->nodes[l]->freq < h->nodes[smallest]->freq) smallest = l;
        if (r < h->size && h->nodes[r]->freq < h->nodes[smallest]->freq) smallest = r;
        if (smallest == i) break;
        HuffmanNode *tmp = h->nodes[i];
        h->nodes[i] = h->nodes[smallest];
        h->nodes[smallest] = tmp;
        i = smallest;
    }
    return top;
}

/* -------- Build Huffman tree from frequency array -------- */

void build_huffman_tree(int *frequencies, HuffmanNode **root) {
    MinHeap heap = { .size = 0 };

    /* Create a leaf node for every symbol with frequency > 0 */
    for (int c = 0; c < 256; c++) {
        if (frequencies[c] > 0) {
            HuffmanNode *n = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
            n->symbol = (unsigned char)c;
            n->freq   = frequencies[c];
            heap_push(&heap, n);
        }
    }

    /* Edge case: single unique symbol */
    if (heap.size == 1) {
        HuffmanNode *only = heap_pop(&heap);
        HuffmanNode *dummy = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
        dummy->freq = 0;
        HuffmanNode *parent = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
        parent->freq  = only->freq;
        parent->left  = only;
        parent->right = dummy;
        *root = parent;
        return;
    }

    /* Merge nodes until one root remains */
    while (heap.size > 1) {
        HuffmanNode *a = heap_pop(&heap);
        HuffmanNode *b = heap_pop(&heap);
        HuffmanNode *merged = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
        merged->freq  = a->freq + b->freq;
        merged->left  = a;
        merged->right = b;
        heap_push(&heap, merged);
    }

    *root = heap_pop(&heap);
}

/* -------- Traverse tree to get code lengths -------- */

static unsigned char g_lengths[256];  /* code length per symbol */

static void assign_lengths(HuffmanNode *node, int depth) {
    if (!node) return;
    if (!node->left && !node->right) {
        /* leaf */
        g_lengths[node->symbol] = (unsigned char)(depth < 255 ? depth : 255);
        return;
    }
    assign_lengths(node->left,  depth + 1);
    assign_lengths(node->right, depth + 1);
}

/* -------- Generate canonical codes from tree -------- */

void generate_canonical_codes(HuffmanNode *root, HuffmanCode *codes) {
    memset(g_lengths, 0, 256);
    memset(codes,      0, 256 * sizeof(HuffmanCode));

    if (!root) return;

    assign_lengths(root, 0);

    /* Assign canonical codes:
     *   Sort symbols by (length, symbol); assign codes starting from 0,
     *   incrementing, and left-shifting when length increases.            */
    uint32_t code = 0;
    int max_len = 0;
    for (int i = 0; i < 256; i++) if (g_lengths[i] > max_len) max_len = g_lengths[i];

    for (int len = 1; len <= max_len; len++) {
        for (int sym = 0; sym < 256; sym++) {
            if (g_lengths[sym] == len) {
                codes[sym].code   = code;
                codes[sym].length = (unsigned char)len;
                code++;
            }
        }
        code <<= 1;
    }
}

/* -------- Write header (256 code lengths, 1 byte each) -------- */

void write_header(HuffmanCode *codes, unsigned char *output, size_t *out_len) {
    /* Header format:
     *   [256 bytes]  code length for symbol 0..255
     *                (0 = symbol not present)
     */
    for (int i = 0; i < 256; i++)
        output[(*out_len)++] = codes[i].length;
}

/* -------- Encode data using generated codes -------- */

void encode_data(unsigned char *input, size_t len,
                 HuffmanCode *codes, unsigned char *output, size_t *out_len) {
    unsigned char cur_byte = 0;
    int           bits_left = 8;  /* bits remaining in cur_byte */

    for (size_t i = 0; i < len; i++) {
        unsigned char sym    = input[i];
        uint32_t       code  = codes[sym].code;
        int            clen  = codes[sym].length;

        /* Write clen bits from MSB of code */
        for (int b = clen - 1; b >= 0; b--) {
            int bit = (code >> b) & 1;
            cur_byte = (unsigned char)((cur_byte << 1) | bit);
            bits_left--;
            if (bits_left == 0) {
                output[(*out_len)++] = cur_byte;
                cur_byte  = 0;
                bits_left = 8;
            }
        }
    }

    /* Flush remaining bits (pad with 0s) */
    if (bits_left < 8) {
        cur_byte = (unsigned char)(cur_byte << bits_left);
        output[(*out_len)++] = cur_byte;
    }

    /* Store number of valid bits in last byte as an extra trailer byte */
    output[(*out_len)++] = (unsigned char)((bits_left == 8) ? 0 : (8 - bits_left));
    /* Note: if bits_left==8 the last byte was already flushed; trailer=0 means 8 bits used */
}

/* -------- High-level huffman_encode -------- */

void huffman_encode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len) {
    *out_len = 0;
    if (!input || len == 0) return;

    /* Count frequencies */
    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[input[i]]++;

    /* Build tree */
    HuffmanNode *root = NULL;
    build_huffman_tree(freq, &root);

    /* Generate canonical codes */
    HuffmanCode codes[256];
    generate_canonical_codes(root, codes);
    free_huffman_tree(root);

    /* Write header */
    write_header(codes, output, out_len);

    /* Store original length (4 bytes, little-endian) */
    uint32_t orig_len32 = (uint32_t)len;
    output[(*out_len)++] = (orig_len32)       & 0xFF;
    output[(*out_len)++] = (orig_len32 >> 8)  & 0xFF;
    output[(*out_len)++] = (orig_len32 >> 16) & 0xFF;
    output[(*out_len)++] = (orig_len32 >> 24) & 0xFF;

    /* Encode data */
    encode_data(input, len, codes, output, out_len);
}

/* -------- Decode header and reconstruct canonical codes -------- */

static void rebuild_codes_from_lengths(unsigned char *lengths, HuffmanCode *codes) {
    memset(codes, 0, 256 * sizeof(HuffmanCode));

    int max_len = 0;
    for (int i = 0; i < 256; i++) if (lengths[i] > max_len) max_len = lengths[i];

    uint32_t code = 0;
    for (int len = 1; len <= max_len; len++) {
        for (int sym = 0; sym < 256; sym++) {
            if (lengths[sym] == len) {
                codes[sym].code   = code;
                codes[sym].length = (unsigned char)len;
                code++;
            }
        }
        code <<= 1;
    }
}

/* -------- High-level huffman_decode -------- */

void huffman_decode(unsigned char *input, size_t len,
                    unsigned char *output, size_t *out_len) {
    *out_len = 0;
    if (!input || len < 256 + 4 + 1) return;

    /* Read 256-byte header */
    unsigned char lengths[256];
    memcpy(lengths, input, 256);
    size_t pos = 256;

    /* Read original length */
    uint32_t orig_len32 =  (uint32_t)input[pos]
                        | ((uint32_t)input[pos+1] << 8)
                        | ((uint32_t)input[pos+2] << 16)
                        | ((uint32_t)input[pos+3] << 24);
    pos += 4;

    /* Rebuild canonical codes */
    HuffmanCode codes[256];
    rebuild_codes_from_lengths(lengths, codes);

    /* Build a decoding tree for fast bit-by-bit decoding */
    HuffmanNode *decode_root = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
    for (int sym = 0; sym < 256; sym++) {
        if (codes[sym].length > 0) {
            HuffmanNode *curr = decode_root;
            for (int b = codes[sym].length - 1; b >= 0; b--) {
                int bit = (codes[sym].code >> b) & 1;
                if (bit == 0) {
                    if (!curr->left) curr->left = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
                    curr = curr->left;
                } else {
                    if (!curr->right) curr->right = (HuffmanNode *)calloc(1, sizeof(HuffmanNode));
                    curr = curr->right;
                }
            }
            curr->symbol = (unsigned char)sym;
            curr->freq = 1; /* Mark as leaf */
        }
    }

    size_t data_end   = len - 1;              /* last byte is trailer */
    unsigned char valid_bits = input[len - 1]; /* bits used in last byte */
    if (valid_bits == 0) valid_bits = 8;

    size_t decoded = 0;
    HuffmanNode *curr = decode_root;

    for (size_t i = pos; i < data_end && decoded < orig_len32; i++) {
        unsigned char byte = input[i];
        int bits_in_byte = (i == data_end - 1) ? (int)valid_bits : 8;

        for (int b = 7; b >= 8 - bits_in_byte && decoded < orig_len32; b--) {
            int bit = (byte >> b) & 1;
            curr = (bit == 0) ? curr->left : curr->right;
            if (!curr) { /* Should not happen with valid Huffman stream */
                curr = decode_root;
                continue;
            }

            if (!curr->left && !curr->right && curr->freq == 1) {
                output[decoded++] = curr->symbol;
                curr = decode_root;
            }
        }
    }

    free_huffman_tree(decode_root);
    *out_len = decoded;
}

/* -------- Free Huffman tree recursively -------- */

void free_huffman_tree(HuffmanNode *node) {
    if (!node) return;
    free_huffman_tree(node->left);
    free_huffman_tree(node->right);
    free(node);
}
