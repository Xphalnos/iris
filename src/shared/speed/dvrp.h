#ifndef DVRP_H
#define DVRP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#include "../speed.h"

#define DVRP_INTR_READY 0x01
#define DVRP_INTR_CMD_ACK 0x02
#define DVRP_INTR_CMD_COMP 0x04
#define DVRP_INTR_DMA_ACK 0x08
#define DVRP_INTR_DMA_END 0x10

struct ps2_dvrp {
    // 0x3109 - avioctl2_set_d_audio_sel
    uint16_t cmd;

    // Unknown what these parameters mean
    uint16_t params[0x10];
    uint8_t param_index;

    // bit 1 - Busy
    // bit 2 - DVR/MISC task
    // bit 3 - AV task
    // bit 4 - DVR task
    // bit 5 - IOMAN task
    uint8_t status;

    // bit 7 - busy? (expected to be 0)
    uint8_t status2;

    // DVR INTR regs:
    // 4200 - intr stat (R?)
    // 4204 - intr ack (W?)
    // 4208 - intr mask (RW)
    // 4220 - intr cause/command? (R?)

    // DVR INTRs:
    // bit 0 - DVRRDY (DVR ready)
    // bit 1 - CMD_ACK (Command acknowledged)
    // bit 2 - CMD_COMP (Command completed)
    // bit 3 - DMAACK (DMA transfer acknowledged)
    // bit 4 - DMAEND (DMA transfer ended)
    uint16_t intr_stat; // 4200h
    uint16_t intr_mask; // 4208h
    uint16_t intr_cause; // 4220h Command that caused the interrupt?

    struct ps2_speed* speed;
};

struct ps2_dvrp* ps2_dvrp_create(void);
void ps2_dvrp_init(struct ps2_dvrp* dvrp, struct ps2_speed* speed);
void ps2_dvrp_destroy(struct ps2_dvrp* dvrp);
uint64_t ps2_dvrp_read(struct ps2_dvrp* dvrp, uint32_t addr);
void ps2_dvrp_write(struct ps2_dvrp* dvrp, uint32_t addr, uint64_t data);

#ifdef __cplusplus
}
#endif

#endif