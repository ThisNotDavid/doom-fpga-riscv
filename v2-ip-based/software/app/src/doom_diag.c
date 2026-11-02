#include "doom_diag.h"
#include "system.h"
#include "io.h"
#include "sys/alt_irq.h"
#include "sys/alt_exceptions.h"

#ifndef ALT_INCLUDE_INSTRUCTION_RELATED_EXCEPTION_API
#error Enable the BSP instruction-related exception API
#endif

volatile DoomFault doom_fault;

extern volatile uint32_t doom_audio_interrupts;
extern volatile uint32_t doom_audio_frames;
extern volatile uint32_t doom_audio_empty_observations;

static void capture(uint32_t kind, uint32_t cause,
                    uint32_t pc, uint32_t bad_address,
                    const char *message)
    __attribute__((noreturn));

static void capture(uint32_t kind, uint32_t cause,
                    uint32_t pc, uint32_t bad_address,
                    const char *message)
{
    (void)alt_irq_disable_all();
    __asm__ volatile ("" ::: "memory");

    doom_fault.ready = 0;
    doom_fault.kind = kind;
    doom_fault.cause = cause;
    doom_fault.pc = pc;
    doom_fault.bad_address = bad_address;

    doom_fault.audio_interrupts = doom_audio_interrupts;
    doom_fault.audio_frames = doom_audio_frames;
    doom_fault.audio_empty = doom_audio_empty_observations;

    unsigned int i = 0;

    if (message != 0) {
        while (i < sizeof(doom_fault.message) - 1u &&
               message[i] != '\0') {
            doom_fault.message[i] = message[i];
            ++i;
        }
    }

    doom_fault.message[i] = '\0';

    /* Publish the record before touching peripheral registers. */
    __asm__ volatile ("fence rw,rw" ::: "memory");
    doom_fault.ready = 0x444F4F4Du;

    /* Disable playback interrupts and clear the audio FIFOs. */
    IOWR_32DIRECT(AUDIO_BASE, 0, 8u);

    /* E1 = CPU exception; E2 = engine error. */
    IOWR_32DIRECT(LED_PIO_BASE, 0, 0xE0u | kind);

    for (;;) {
        __asm__ volatile ("nop");
    }
}

static alt_exception_result exception_handler(
    alt_exception_cause cause,
    alt_u32 exception_pc,
    alt_u32 bad_address)
{
    capture(1u, (uint32_t)cause, exception_pc,
            bad_address, "CPU exception");
}

void DoomDiag_Init(void)
{
    doom_fault.ready = 0;
    alt_instruction_exception_register(exception_handler);
}

void DoomDiag_Fatal(const char *message)
{
    /* This PC identifies the caller, not a hardware exception. */
    uint32_t caller =
        (uint32_t)(uintptr_t)__builtin_return_address(0);

    capture(2u, 0u, caller, 0u, message);
}