#include <stdint.h>
#include <stdio.h>
#include "system.h"
#include "sys/alt_cache.h"
#include "sdram_test.h"

#define TEST_WORDS 65536u

/* Dedicated test storage in SDRAM, aligned to a cache line. */
static volatile uint32_t test_memory[TEST_WORDS]
    __attribute__((aligned(ALT_CPU_DCACHE_LINE_SIZE)));

static uint32_t test_pattern(unsigned pass, uint32_t index)
{
    switch (pass) {
    case 0:
        return 0x00000000u;

    case 1:
        return 0xFFFFFFFFu;

    default:
        return (index * 0x9E3779B9u) ^ 0xA5A5A5A5u;
    }
}

int sdram_test_run(void)
{
    unsigned pass;
    uint32_t i;
    uint32_t expected;
    uint32_t actual;

    printf("SDRAM: testing 256 KiB at 0x%08lx\n",
           (unsigned long)(uintptr_t)test_memory);

    for (pass = 0; pass < 3; ++pass) {
        for (i = 0; i < TEST_WORDS; ++i) {
            test_memory[i] = test_pattern(pass, i);
        }

        /* Write dirty cache contents to external memory first. */
        alt_dcache_flush_all();

        /* Discard cached copies so verification fetches SDRAM data. */
        alt_dcache_flush_no_writeback(
            (void *)test_memory,
            sizeof(test_memory)
        );

        for (i = 0; i < TEST_WORDS; ++i) {
            expected = test_pattern(pass, i);
            actual = test_memory[i];

            if (actual != expected) {
                printf(
                    "FAIL SDRAM: pass %u address 0x%08lx "
                    "expected 0x%08lx got 0x%08lx\n",
                    pass + 1,
                    (unsigned long)(uintptr_t)&test_memory[i],
                    (unsigned long)expected,
                    (unsigned long)actual
                );
                return -1;
            }
        }

        printf("PASS SDRAM pattern %u/3\n", pass + 1);
    }

    puts("PASS SDRAM: 256 KiB, three patterns");
    return 0;
}