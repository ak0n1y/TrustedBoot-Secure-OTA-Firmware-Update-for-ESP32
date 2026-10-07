#pragma once

#include <stdint.h>

#define FWMETA_OFFSET   0x18000
#define FWMETA_SIZE     0x1000
#define FWMETA_SUBTYPE  0x40
#define FWMETA_SLOTS    2
#define FWMETA_MAGIC    0x4C415445u

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t reserved;
    uint8_t sha256[32];
    uint8_t padding[24];
} fwmeta_record_t;

_Static_assert(sizeof(fwmeta_record_t) == 64, "fwmeta_record_t must be 64 bytes");
