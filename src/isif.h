#ifndef ISIF_H
#define ISIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#define ISIF_MAGIC 0x46495349 // "ISIF"
#define ISIF_BLOCK_MODE_UNCOMPRESSED_32BIT 0
#define ISIF_BLOCK_MODE_UNCOMPRESSED_64BIT 1
#define ISIF_BLOCK_MODE_COMPRESSED_32BIT 2
#define ISIF_BLOCK_MODE_COMPRESSED_64BIT 3

struct isif_header {
    uint32_t magic;
    uint32_t version;
    uint64_t block_count;
    uint32_t block_size;
    uint16_t block_mode;
    uint16_t block_compression;
    uint64_t bat_offset;
    uint64_t extension_offset;
    uint64_t data_offset;
    uint64_t bat_block_count;
    uint64_t reserved;
};

struct isif_state {
    FILE* file;

    struct isif_header hdr;
    void* bat;
};

struct isif_state* isif_open(const char* path);
uint32_t isif_get_version(struct isif_state* state);
uint64_t isif_get_block_count(struct isif_state* state);
uint32_t isif_get_block_size(struct isif_state* state);
uint16_t isif_get_block_mode(struct isif_state* state);
uint16_t isif_get_block_compression(struct isif_state* state);
uint64_t isif_get_total_size(struct isif_state* state);
uint64_t isif_get_allocated_size(struct isif_state* state);
void isif_read_extension(struct isif_state* state, void* buffer);
void isif_read_block(struct isif_state* state, uint64_t index, void* buffer);
void isif_write_block(struct isif_state* state, uint64_t index, const void* buffer);
void isif_close(struct isif_state* state);

// File utility functions
int isif_create_image(const char* path, uint64_t block_count, uint32_t block_size, uint16_t block_mode, uint16_t block_compression, void* ext, uint64_t ext_size);

#ifdef __cplusplus
}
#endif

#endif