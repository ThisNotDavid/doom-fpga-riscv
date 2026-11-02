#include <stdint.h>
#include <stdio.h>
#include "doom_hw.h"
#include "vga.h"

static int wait_for_swap(void)
{
    uint32_t start;

    start = IORD_32DIRECT(IO_BASE, DOOM_TIMER_OFFSET);

    while (IORD_32DIRECT(IO_BASE, DOOM_STATUS_OFFSET)
           & DOOM_STATUS_PENDING) {
        uint32_t now;

        now = IORD_32DIRECT(IO_BASE, DOOM_TIMER_OFFSET);

        if ((uint32_t)(now - start) >= 100u) {
            puts("FAIL VGA: frame swap timeout");
            return -1;
        }
    }

    return 0;
}

int vga_show_test_pattern(void)
{
    /* Palette entries use 0x00RRGGBB. */
    static const uint32_t colours[8] = {
        0x00FFFFFFu, /* White   */
        0x00FFFF00u, /* Yellow  */
        0x0000FFFFu, /* Cyan    */
        0x0000FF00u, /* Green   */
        0x00FF00FFu, /* Magenta */
        0x00FF0000u, /* Red     */
        0x000000FFu, /* Blue    */
        0x00000000u  /* Black   */
    };

    uint32_t status;
    uint32_t bank;
    uint32_t framebuffer;
    uint32_t palette;
    uint32_t x;
    uint32_t y;
    uint32_t i;

    if (wait_for_swap() != 0) {
        return -1;
    }

    status = IORD_32DIRECT(IO_BASE, DOOM_STATUS_OFFSET);

    /* Write the opposite bank from the one being displayed. */
    bank = (status & DOOM_STATUS_FRONT_BANK) ^ 1u;

    framebuffer = DOOM_FB_BASE_OFFSET
                + bank * DOOM_FB_BANK_BYTES;

    palette = DOOM_PALETTE_BASE_OFFSET
            + bank * DOOM_PALETTE_BANK_BYTES;

    for (i = 0; i < 256u; ++i) {
        uint32_t rgb = (i < 8u) ? colours[i] : 0u;

        IOWR_32DIRECT(IO_BASE, palette + i * 4u, rgb);
    }

    for (y = 0; y < DOOM_SCREEN_HEIGHT; ++y) {
        for (x = 0; x < DOOM_SCREEN_WIDTH; x += 4u) {
            uint32_t index;
            uint32_t pixels;
            uint32_t offset;

            /* Each bar is 40 pixels wide, divisible by four. */
            index = x / (DOOM_SCREEN_WIDTH / 8u);

            /* Pack four identical 8-bit palette indices. */
            pixels = index * 0x01010101u;
            offset = framebuffer + y * DOOM_SCREEN_WIDTH + x;

            IOWR_32DIRECT(IO_BASE, offset, pixels);
        }
    }

    IOWR_32DIRECT(
        IO_BASE,
        DOOM_SUBMIT_OFFSET,
        bank | DOOM_SUBMIT_ENABLE
    );

    if (wait_for_swap() != 0) {
        return -1;
    }

    status = IORD_32DIRECT(IO_BASE, DOOM_STATUS_OFFSET);

    if ((status & DOOM_STATUS_FRONT_BANK) != bank ||
        !(status & DOOM_STATUS_ENABLED)) {
        printf("FAIL VGA: unexpected status 0x%08lx\n",
               (unsigned long)status);
        return -1;
    }

    printf("PASS VGA: colour bars submitted to bank %lu\n",
           (unsigned long)bank);

    return 0;
}