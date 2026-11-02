#include <stdint.h>
#include <stdio.h>

#include "system.h"
#include "io.h"
#include "sys/alt_alarm.h"

#include "doomgeneric.h"
#include "i_video.h"
#include "i_system.h"
#include "ps2_keys.h"
#include "ramfs.h"
#include "niosv_fs.h"
#include "doom_diag.h"

#define FB_WIDTH   320u
#define FB_HEIGHT  240u
#define FB_FRONT   0x27F00000u
#define FB_BACK    0x27F40000u

extern const uint8_t embedded_wad[];
extern const unsigned int embedded_wad_size;

static Ps2Parser parser;
static uint8_t held[256];

uint32_t DG_GetTicksMs(void)
{

    uint32_t frequency = alt_ticks_per_second();

    return (uint32_t)
        (((uint64_t)alt_nticks() * 1000u) / frequency);
}

void DG_SleepMs(uint32_t ms)
{
    uint32_t start = DG_GetTicksMs();

    while ((uint32_t)(DG_GetTicksMs() - start) < ms) {
    }
}

static void wait_for_swap(void)
{
    uint32_t start = DG_GetTicksMs();

    while (IORD_32DIRECT(PIXEL_DMA_BASE, 12) & 1u) {
        if ((uint32_t)(DG_GetTicksMs() - start) >= 1000u)
            I_Error("VGA buffer swap timeout");
    }
}

static void clear_buffer(uintptr_t base)
{
    volatile uint16_t *pixels = (volatile uint16_t *)base;

    for (unsigned int i = 0; i < FB_WIDTH * FB_HEIGHT; ++i)
        pixels[i] = 0;
}

void DG_Init(void)
{
    if (DG_ScreenBuffer == NULL)
        I_Error("Cannot allocate DOOM screen buffer");

    if (alt_ticks_per_second() == 0)
        I_Error("HAL system timer is unavailable");

    /* Use polling, with receive interrupts disabled. */
    IOWR_32DIRECT(PS2_BASE, 4, 0u);

    wait_for_swap();

    if (IORD_32DIRECT(PIXEL_DMA_BASE, 8)
        != ((FB_HEIGHT << 16) | FB_WIDTH))
        I_Error("Expected a 320x240 VGA framebuffer");

    uint32_t front = IORD_32DIRECT(PIXEL_DMA_BASE, 0);
    uint32_t back = IORD_32DIRECT(PIXEL_DMA_BASE, 4);

    if (!((front == FB_FRONT && back == FB_BACK) ||
          (front == FB_BACK && back == FB_FRONT)))
        I_Error("Unexpected VGA framebuffer addresses");

    clear_buffer(FB_FRONT);
    clear_buffer(FB_BACK);

    __asm__ volatile ("fence iorw,iorw" ::: "memory");

    puts("DOOM: RGB565 VGA, PS/2 polling, RAM filesystem");
}

/* FPS overlay: 3x5 font, enlarged to 6x10 framebuffer pixels. */
static const uint8_t fps_digits[10][5] = {
    {7, 5, 5, 5, 7}, /* 0 */
    {2, 6, 2, 2, 7}, /* 1 */
    {7, 1, 7, 4, 7}, /* 2 */
    {7, 1, 7, 1, 7}, /* 3 */
    {5, 5, 7, 1, 1}, /* 4 */
    {7, 4, 7, 1, 7}, /* 5 */
    {7, 4, 7, 5, 7}, /* 6 */
    {7, 1, 1, 1, 1}, /* 7 */
    {7, 5, 7, 5, 7}, /* 8 */
    {7, 5, 7, 1, 7}  /* 9 */
};

static uint8_t fps_glyph_row(char c, unsigned int row)
{
    static const uint8_t letter_f[5] = {7, 4, 6, 4, 4};
    static const uint8_t letter_p[5] = {7, 5, 7, 4, 4};
    static const uint8_t letter_s[5] = {7, 4, 7, 1, 7};

    if (c >= '0' && c <= '9')
        return fps_digits[c - '0'][row];

    switch (c) {
    case 'F': return letter_f[row];
    case 'P': return letter_p[row];
    case 'S': return letter_s[row];
    case ':': return (row == 1 || row == 3) ? 2u : 0u;
    case '.': return row == 4 ? 2u : 0u;
    case '-': return row == 2 ? 7u : 0u;
    default:  return 0;
    }
}

static char fps_text[16] = "FPS: --.-";
static unsigned int fps_swap_pending;
static unsigned int fps_started;
static uint32_t fps_start_tick;
static uint32_t fps_frame_count;

/* Optional debugger measurements for the latest completed window. */
volatile uint32_t doom_fps_x10;
volatile uint32_t doom_fps_window_frames;
volatile uint32_t doom_fps_window_ticks;

static void fps_observe_swap(void)
{
    if (!fps_swap_pending)
        return;

    fps_swap_pending = 0;

    uint32_t now = (uint32_t)alt_nticks();

    /* Use the first observed completion as the timing origin. */
    if (!fps_started) {
        fps_started = 1;
        fps_start_tick = now;
        fps_frame_count = 0;
        return;
    }

    ++fps_frame_count;

    uint32_t elapsed = now - fps_start_tick;
    uint32_t frequency = alt_ticks_per_second();

    if (frequency != 0 && elapsed >= frequency) {
        uint32_t rate_x10 = (uint32_t)(
            ((uint64_t)fps_frame_count * frequency * 10u
             + elapsed / 2u) / elapsed
        );

        doom_fps_x10 = rate_x10;
        doom_fps_window_frames = fps_frame_count;
        doom_fps_window_ticks = elapsed;

        snprintf(fps_text, sizeof(fps_text), "FPS: %lu.%lu",
                 (unsigned long)(rate_x10 / 10u),
                 (unsigned long)(rate_x10 % 10u));

        fps_frame_count = 0;
        fps_start_tick = now;
    }
}

static void fps_draw_overlay(volatile uint16_t *pixels)
{
    enum {
        BOX_WIDTH = 88,
        BOX_HEIGHT = 14,
        X = FB_WIDTH - BOX_WIDTH - 4,
        Y = 4,
        SCALE = 2
    };

    /* Solid background keeps the text readable over every scene. */
    for (unsigned int y = 0; y < BOX_HEIGHT; ++y)
        for (unsigned int x = 0; x < BOX_WIDTH; ++x)
            pixels[(Y + y) * FB_WIDTH + X + x] = 0x0000;

    for (unsigned int i = 0; fps_text[i] != '\0' && i < 10; ++i) {
        unsigned int origin_x = X + 2u + i * 8u;

        for (unsigned int row = 0; row < 5; ++row) {
            uint8_t bits = fps_glyph_row(fps_text[i], row);

            for (unsigned int col = 0; col < 3; ++col) {
                if (!(bits & (1u << (2u - col))))
                    continue;

                for (unsigned int dy = 0; dy < SCALE; ++dy)
                    for (unsigned int dx = 0; dx < SCALE; ++dx)
                        pixels[
                            (Y + 2u + row * SCALE + dy) * FB_WIDTH
                            + origin_x + col * SCALE + dx
                        ] = 0xFFFF;
            }
        }
    }
}

void DG_DrawFrame(void)
{
    uint16_t palette[256];

    wait_for_swap();
    fps_observe_swap();

    uintptr_t base = IORD_32DIRECT(PIXEL_DMA_BASE, 4);
    volatile uint16_t *destination = (volatile uint16_t *)base;

    for (unsigned int i = 0; i < 256; ++i) {
        palette[i] =
            ((uint16_t)(colors[i].r >> 3) << 11) |
            ((uint16_t)(colors[i].g >> 2) << 5) |
            ((uint16_t)(colors[i].b >> 3));
    }

    /*
     * Expand DOOM's 320x200 image to the existing 320x240
     * framebuffer. The hardware scaler then produces 640x480.
     */
    for (unsigned int y = 0; y < FB_HEIGHT; ++y) {
        unsigned int source_y = y * 200u / FB_HEIGHT;
        const pixel_t *source = DG_ScreenBuffer + source_y * 320u;

        for (unsigned int x = 0; x < FB_WIDTH; ++x)
            destination[y * FB_WIDTH + x] = palette[source[x]];
    }

    /* Draw after the game image, into the same back buffer. */
    fps_draw_overlay(destination);

    /* Finish framebuffer writes before requesting presentation. */
    __asm__ volatile ("fence iorw,iorw" ::: "memory");

    IOWR_32DIRECT(PIXEL_DMA_BASE, 0, 1u);
    fps_swap_pending = 1;

}

int DG_GetKey(int *pressed, unsigned char *key)
{
    for (;;) {
        uint32_t data = IORD_32DIRECT(PS2_BASE, 0);

        if ((data & 0x8000u) == 0u)
            return 0;

        if (ps2_key(&parser, (uint8_t)data, pressed, key)) {
            /* Ignore keyboard repeat while the key remains held. */
            if (held[*key] == (uint8_t)*pressed)
                continue;

            held[*key] = (uint8_t)*pressed;
            return 1;
        }
    }
}

void DG_SetWindowTitle(const char *title)
{
    printf("DOOM: %s\n", title);
}

int main(void)
{
    DoomDiag_Init();

    char *argv[] = {
        "doom",
        "-iwad", "/ram/DOOM.WAD",
        "-nomusic",
        "-mb", "16",
        "-config", "/ram/default.cfg",
        "-extraconfig", "/ram/extra.cfg",
        "-savedir", "/ram/",
        NULL
    };

    setvbuf(stdout, NULL, _IONBF, 0);
    puts("Starting DOOM");

    niosv_fs_init();

    if (ramfs_mount_wad("/ram/DOOM.WAD",
                        embedded_wad, embedded_wad_size) < 0) {
        puts("Cannot mount embedded WAD");
        return 1;
    }

    printf("Embedded WAD: %u bytes\n", embedded_wad_size);

    doomgeneric_Create(
        (int)(sizeof(argv) / sizeof(argv[0])) - 1,
        argv
    );

    for (;;) {
        doomgeneric_Tick();
    }
}
