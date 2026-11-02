#include <stdint.h>
#include "system.h"
#include "io.h"
#include "sys/alt_alarm.h"
#include "sys/alt_stdio.h"

/* Values available for inspection in the debugger. */
volatile uint32_t audio_test_stage;
volatile uint32_t audio_test_status;
volatile uint32_t audio_test_frames;

static uint32_t milliseconds(void)
{
    return (uint32_t)(((uint64_t)alt_nticks() * 1000u)
                      / alt_ticks_per_second());
}

static void fail(const char *message)
{
    /* Clear playback FIFOs and disable interrupts. */
    IOWR_32DIRECT(AUDIO_BASE, 0, 8u);
    audio_test_stage = 0xFFu;
    alt_putstr(message);

    for (;;) {
    }
}

int main(void)
{
    unsigned int left_phase = 0;
    unsigned int right_phase = 0;
    uint32_t start;

    audio_test_stage = 1;
    alt_putstr("\nAudio test: waiting for codec configuration\n");

    if (alt_ticks_per_second() == 0)
        fail("ERROR: system timer unavailable\n");

    start = milliseconds();

    for (;;) {
        uint32_t status = IORD_32DIRECT(AV_CONFIG_BASE, 4);
        audio_test_status = status;

        /* Bit 8: automatic configuration completed successfully. */
        if (status & 0x100u)
            break;

        if ((uint32_t)(milliseconds() - start) >= 3000u) {
            alt_printf("AV_CONFIG status=%x\n", status);
            fail("ERROR: automatic configuration did not succeed\n");
        }
    }

    /* Clear both playback FIFOs, then release clear.
       Interrupt enables remain zero. */
    IOWR_32DIRECT(AUDIO_BASE, 0, 8u);
    IOWR_32DIRECT(AUDIO_BASE, 0, 0u);
    __asm__ volatile ("fence iorw,iorw" ::: "memory");

    audio_test_stage = 2;
    alt_putstr("Playing: left ~1 kHz, right ~500 Hz\n");

    uint32_t last_progress = milliseconds();

    for (;;) {
        uint32_t space = IORD_32DIRECT(AUDIO_BASE, 4);
        unsigned int left_space = (space >> 24) & 0xFFu;
        unsigned int right_space = (space >> 16) & 0xFFu;
        unsigned int count =
            left_space < right_space ? left_space : right_space;

        audio_test_status = space;

        if (count == 0) {
            if ((uint32_t)(milliseconds() - last_progress) >= 1000u)
                fail("ERROR: playback FIFO stopped draining\n");

            continue;
        }

        for (unsigned int i = 0; i < count; ++i) {
            /* Low-amplitude signed 16-bit square waves. */
            int32_t left = left_phase < 24u ? 2048 : -2048;
            int32_t right = right_phase < 48u ? 2048 : -2048;

            IOWR_32DIRECT(AUDIO_BASE, 8, (uint32_t)left);
            IOWR_32DIRECT(AUDIO_BASE, 12, (uint32_t)right);

            if (++left_phase == 48u)
                left_phase = 0;

            if (++right_phase == 96u)
                right_phase = 0;

            ++audio_test_frames;
        }

        last_progress = milliseconds();
        audio_test_stage = 3;
    }
}