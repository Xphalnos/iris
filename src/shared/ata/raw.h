#ifndef RAW_H
#define RAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

struct raw_state {
    FILE* file;

    uint64_t sector_count;
};

struct raw_state* raw_open(const char* path);
uint64_t raw_get_sector_count(struct raw_state* state);
void raw_read_sector(struct raw_state* state, uint64_t lba, uint8_t* buf);
void raw_write_sector(struct raw_state* state, uint64_t lba, const uint8_t* buf);
int raw_get_identify(struct raw_state* state, uint8_t* buf);
void raw_close(struct raw_state* state);

// ATA adapters
void ata_raw_read_sector(void* udata, uint64_t lba, uint8_t* buf);
void ata_raw_write_sector(void* udata, uint64_t lba, const uint8_t* buf);
int ata_raw_get_identify(void* udata, uint8_t* buf);
uint64_t ata_raw_get_sector_count(void* udata);
void ata_raw_close(void* udata);

#ifdef __cplusplus
}
#endif

#endif