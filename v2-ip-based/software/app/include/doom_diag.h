#ifndef DOOM_DIAG_H
#define DOOM_DIAG_H

#include <stdint.h>

typedef struct {
    uint32_t ready;
    uint32_t kind;       /* 1 = CPU exception, 2 = engine error */
    uint32_t cause;
    uint32_t pc;
    uint32_t bad_address;
    uint32_t audio_interrupts;
    uint32_t audio_frames;
    uint32_t audio_empty;
    char message[192];
} DoomFault;

extern volatile DoomFault doom_fault;

void DoomDiag_Init(void);

void DoomDiag_Fatal(const char *message)
    __attribute__((noreturn));

#endif