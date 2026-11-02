#ifndef DOOM_HW_H
#define DOOM_HW_H

#include "system.h"
#include "io.h"

/* Byte offsets relative to IO_BASE. */
#define DOOM_STATUS_OFFSET   0x21000u
#define DOOM_SUBMIT_OFFSET   0x21004u
#define DOOM_TIMER_OFFSET    0x21008u
#define DOOM_KEY_OFFSET      0x2100Cu
#define DOOM_ERRORS_OFFSET   0x21010u
#define DOOM_ID_OFFSET       0x21014u

#define DOOM_EXPECTED_ID     0x444F4F4Du

// VGA
#define DOOM_FB_BASE_OFFSET       0x00000u
#define DOOM_FB_BANK_BYTES        0x10000u
#define DOOM_PALETTE_BASE_OFFSET  0x20000u
#define DOOM_PALETTE_BANK_BYTES   0x00400u

#define DOOM_SCREEN_WIDTH        320u
#define DOOM_SCREEN_HEIGHT       200u

#define DOOM_STATUS_FRONT_BANK   0x01u
#define DOOM_STATUS_PENDING      0x02u
#define DOOM_STATUS_ENABLED      0x04u

#define DOOM_SUBMIT_ENABLE       0x02u

// PS/2 keyboard
#define DOOM_KEY_VALID          0x100u
#define DOOM_KEY_BYTE_MASK      0x0FFu

#define DOOM_KEY_ERROR_OVERFLOW 0x01u
#define DOOM_KEY_ERROR_FRAME    0x02u

#endif // DOOM_HW_H