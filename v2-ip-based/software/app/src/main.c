#include <stdint.h>
#include <stdio.h>
#include "system.h"
#include "io.h"
#include "doom_hw.h"
#include "sdram_test.h"
#include "vga.h"
#include "keyboard.h"
#include <string.h>
#include "niosv_fs.h"
#include "ramfs.h"
#include "embedded_wad.h"

static int filesystem_test(void);
static int wad_test(void);

int main(void)
{
    uint32_t id;
    uint32_t previous;
    uint32_t now;
    uint8_t key_byte;
    uint32_t key_errors;

    setvbuf(stdout, NULL, _IONBF, 0);

    puts("DOOM: first RiscFree application");

    id = IORD_32DIRECT(IO_BASE, DOOM_ID_OFFSET);
    printf("IO ID: 0x%08lx\n", (unsigned long)id);

    if (id != DOOM_EXPECTED_ID) {
        puts("FAIL: unexpected IO ID");
        for (;;) {
        }
    }

    puts("PASS: IO ID");

    // SDRAM test
    if (sdram_test_run() != 0) {
        puts("Stopped: SDRAM test failed");
        for (;;) {
        }
    }

    // Filesystem test
    niosv_fs_init();

    if (filesystem_test() != 0) {
        puts("Stopped: filesystem test failed");
        for (;;) {
        }
    }

    // WAD test
    if (wad_test() != 0) {
        puts("Stopped: WAD test failed");
        for (;;) {
        }
    }

    // VGA test must be run after SDRAM test, because the VGA test uses SDRAM for the framebuffer.
    if (vga_show_test_pattern() != 0) {
        puts("Stopped: VGA test failed");
        for (;;) {
        }
    }

    // Keyboard test
    puts("Keyboard ready: press and release W, A, S, D.");

    previous = IORD_32DIRECT(IO_BASE, DOOM_TIMER_OFFSET);

    for (;;) {
        key_errors = keyboard_take_errors();

        if (key_errors & DOOM_KEY_ERROR_OVERFLOW) {
            puts("KEYBOARD ERROR: FIFO overflow");
        }

        if (key_errors & DOOM_KEY_ERROR_FRAME) {
            puts("KEYBOARD ERROR: invalid PS/2 frame");
        }

        if (keyboard_read_byte(&key_byte)) {
            printf("KEY %02X\n", (unsigned int)key_byte);
        }

        now = IORD_32DIRECT(IO_BASE, DOOM_TIMER_OFFSET);

        if ((uint32_t)(now - previous) >= 1000u) {
            previous = now;
            printf("Timer: %lu ms\n", (unsigned long)now);
        }
    }
}


static int filesystem_test(void)
{
    static const char expected[] = "DOOM RAM filesystem";
    char actual[sizeof(expected)];
    FILE *file;
    size_t count;
    int close_result;

    file = fopen("/ram/test.txt", "wb");
    if (file == NULL) {
        perror("FAIL RAMFS: open for writing");
        return -1;
    }

    count = fwrite(expected, 1, sizeof(expected), file);
    close_result = fclose(file);

    if (count != sizeof(expected) || close_result != 0) {
        puts("FAIL RAMFS: write or close");
        return -1;
    }

    file = fopen("/ram/test.txt", "rb");
    if (file == NULL) {
        perror("FAIL RAMFS: reopen for reading");
        return -1;
    }

    count = fread(actual, 1, sizeof(actual), file);
    close_result = fclose(file);

    if (count != sizeof(expected) ||
        close_result != 0 ||
        memcmp(actual, expected, sizeof(expected)) != 0) {
        puts("FAIL RAMFS: readback");
        return -1;
    }

    puts("PASS RAMFS: fopen, fwrite, fclose, reopen, fread");
    return 0;
}

static int wad_test(void)
{
    char path[128];
    unsigned char header[12];
    FILE *file;
    size_t count;
    int close_result;

    printf("WAD: %s, %lu bytes\n",
           embedded_wad_name,
           (unsigned long)embedded_wad_size);

    if (embedded_wad_size < sizeof(header)) {
        puts("FAIL WAD: file too small");
        return -1;
    }

    snprintf(path, sizeof(path), "/ram/%s", embedded_wad_name);

    if (ramfs_mount_wad(path, embedded_wad, embedded_wad_size) < 0) {
        puts("FAIL WAD: mount");
        return -1;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        perror("FAIL WAD: open");
        return -1;
    }

    count = fread(header, 1, sizeof(header), file);
    close_result = fclose(file);

    if (count != sizeof(header) ||
        close_result != 0 ||
        memcmp(header, "IWAD", 4) != 0) {
        puts("FAIL WAD: header");
        return -1;
    }

    puts("PASS WAD: mounted and IWAD header readable");
    return 0;
}
