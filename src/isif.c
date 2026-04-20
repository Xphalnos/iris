#include <stdlib.h>
#include <string.h>

#include "isif.h"

#ifdef _MSC_VER
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#elif defined(_WIN32)
#define fseek64 fseeko64
#define ftell64 ftello64
#else
#define fseek64 fseek
#define ftell64 ftell
#endif

static inline int isif_get_bat_entry_size(struct isif_state* isif) {
    if (isif->hdr.block_mode & 1) {
        return 8;
    } else {
        return 4;
    }
}

struct isif_state* isif_open(const char* path) {
    struct isif_state* isif = malloc(sizeof(struct isif_state));

    if (!isif) {
        return NULL;
    }

    isif->file = fopen(path, "r+b");

    if (!isif->file) {
        fprintf(stderr, "isif: Unable to open file\n");

        free(isif);

        return NULL;
    }

    fread(&isif->hdr, sizeof(struct isif_header), 1, isif->file);

    if (isif->hdr.magic != ISIF_MAGIC) {
        fprintf(stderr, "isif: Invalid ISIF header\n");

        fclose(isif->file);
        free(isif);

        return NULL;
    }

    if (isif->hdr.version != 1) {
        fprintf(stderr, "isif: Unsupported ISIF version %d\n", isif->hdr.version);

        fclose(isif->file);
        free(isif);

        return NULL;
    }

    if (isif->hdr.block_mode >= 2) {
        fprintf(stderr, "isif: Unsupported block mode %d\n", isif->hdr.block_mode);

        fclose(isif->file);
        free(isif);

        return NULL;
    }

    // Determine the size of each BAT entry based on the block mode
    int bat_entry_size = isif_get_bat_entry_size(isif);

    // Cache BAT
    isif->bat = malloc(isif->hdr.block_count * bat_entry_size);

    fseek64(isif->file, isif->hdr.bat_offset, SEEK_SET);
    fread(isif->bat, bat_entry_size, isif->hdr.block_count, isif->file);

    return isif;
}

uint32_t isif_get_version(struct isif_state* state) {
    return state->hdr.version;
}

uint64_t isif_get_block_count(struct isif_state* state) {
    return state->hdr.block_count;
}

uint32_t isif_get_block_size(struct isif_state* state) {
    return state->hdr.block_size;
}

uint16_t isif_get_block_mode(struct isif_state* state) {
    return state->hdr.block_mode;
}

uint16_t isif_get_block_compression(struct isif_state* state) {
    return state->hdr.block_compression;
}

uint64_t isif_get_total_size(struct isif_state* state) {
    return state->hdr.block_count * (uint64_t)state->hdr.block_size;
}

uint64_t isif_get_allocated_size(struct isif_state* state) {
    return state->hdr.bat_block_count * (uint64_t)state->hdr.block_size;
}

uint64_t isif_read_bat(struct isif_state* isif, uint64_t index) {
    if (isif->hdr.block_mode & 1) {
        return ((uint64_t*)isif->bat)[index];
    } else {
        return ((uint32_t*)isif->bat)[index];
    }
}

void isif_write_bat(struct isif_state* isif, uint64_t index, uint64_t value) {
    if (isif->hdr.block_mode & 1) {
        ((uint64_t*)isif->bat)[index] = value;
    } else {
        ((uint32_t*)isif->bat)[index] = value;
    }
}

int isif_write_is_empty(struct isif_state* isif, uint8_t* data) {
    for (int i = 0; i < isif->hdr.block_size; i++) {
        if (data[i]) return 0;
    }

    return 1;
}

void isif_allocate_block(struct isif_state* isif, uint64_t index) {
    fseek64(isif->file, 0, SEEK_END);

    uint64_t block_offset = ftell64(isif->file) - isif->hdr.data_offset;

    isif_write_bat(isif, index, block_offset);

    isif->hdr.bat_block_count++;
}

// To-do: Implement shrinking when deallocating blocks
void isif_deallocate_block(struct isif_state* isif, uint64_t index) {
    isif_write_bat(isif, index, 1ull << 63);

    isif->hdr.bat_block_count--;
}

void isif_read_extension(struct isif_state* isif, void* buffer) {
    fseek64(isif->file, isif->hdr.extension_offset, SEEK_SET);
    fread(buffer, 1, isif->hdr.data_offset - isif->hdr.extension_offset, isif->file);
}

void isif_read_block(struct isif_state* isif, uint64_t index, void* buf) {
    if (index >= isif->hdr.block_count) {
        fprintf(stderr, "isif: Block index out of range\n");

        return;
    }

    uint64_t bat_entry = isif_read_bat(isif, index);

    if (bat_entry == (1ull << 63)) {
        memset(buf, 0, isif->hdr.block_size);

        return;
    }

    // Block is unallocated
    if (!bat_entry) {
        memset(buf, 0, isif->hdr.block_size);

        return;
    }

    uint64_t block_offset = isif->hdr.data_offset + bat_entry;

    fseek64(isif->file, block_offset, SEEK_SET);
    fread(buf, 1, isif->hdr.block_size, isif->file);
}

void isif_write_block(struct isif_state* isif, uint64_t index, const void* buf) {
    if (index >= isif->hdr.block_count) {
        fprintf(stderr, "isif: Block index out of range\n");

        return;
    }

    uint64_t bat_entry = isif_read_bat(isif, index);
    int is_empty = isif_write_is_empty(isif, (uint8_t*)buf);
    int is_deallocated = bat_entry == (1ull << 63);

    bat_entry &= ~(1ull << 63);

    // Entry is unallocated
    if (!bat_entry) {
        // Nothing to do here
        if (is_empty)
            return;

        // Allocate new block
        isif_allocate_block(isif, index);

        // Write block data
        fwrite(buf, 1, isif->hdr.block_size, isif->file);

        return;
    }

    // Entry is allocated
    if (is_empty) {
        // Deallocate block
        isif_deallocate_block(isif, index);

        return;
    }

    // Update block
    uint64_t block_offset = isif->hdr.data_offset + bat_entry;

    fseek64(isif->file, block_offset, SEEK_SET);
    fwrite(buf, 1, isif->hdr.block_size, isif->file);

    return;
}

void isif_close(struct isif_state* isif) {
    // Write back BAT
    int bat_entry_size = isif_get_bat_entry_size(isif);
    
    fseek64(isif->file, isif->hdr.bat_offset, SEEK_SET);
    fwrite(isif->bat, bat_entry_size, isif->hdr.block_count, isif->file);

    // Write back header
    fseek64(isif->file, 0, SEEK_SET);
    fwrite(&isif->hdr, sizeof(struct isif_header), 1, isif->file);

    free(isif->bat);
    fclose(isif->file);
    free(isif);
}

int isif_create_image(const char* path, uint64_t block_count, uint32_t block_size, uint16_t block_mode, uint16_t block_compression, void* ext, uint64_t ext_size) {
    FILE* file = fopen(path, "wb");

    if (!file) {
        fprintf(stderr, "isif: Unable to create file\n");

        return 1;
    }

    struct isif_header hdr;

    hdr.magic = ISIF_MAGIC;
    hdr.version = 1;
    hdr.block_count = block_count;
    hdr.block_size = block_size;
    hdr.block_mode = block_mode;
    hdr.block_compression = block_compression;
    hdr.bat_block_count = 0;
    hdr.extension_offset = 0;
    hdr.reserved = 0;

    fwrite(&hdr, sizeof(struct isif_header), 1, file);

    hdr.bat_offset = ftell64(file);

    // Write empty BAT
    int bat_entry_size = (block_mode & 1) ? 8 : 4;

    uint64_t bat_size = block_count * bat_entry_size;

    printf("isif: Creating image with block count %lu, block size %u bat_size=%08x%08x\n", block_count, block_size, (uint32_t)(bat_size >> 32), (uint32_t)(bat_size & 0xFFFFFFFF));

    uint64_t* empty_bat = calloc(block_count, bat_entry_size);
    fwrite(empty_bat, bat_entry_size, block_count, file);
    free(empty_bat);

    if (ext && ext_size > 0) {
        hdr.extension_offset = ftell64(file);

        fwrite(ext, 1, ext_size, file);
    }

    hdr.data_offset = ftell64(file);

    // Update header
    fseek64(file, 0, SEEK_SET);
    fwrite(&hdr, sizeof(struct isif_header), 1, file);

    fclose(file);

    return 0;
}