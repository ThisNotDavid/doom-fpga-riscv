#include "doom_hw.h"
#include "keyboard.h"

int keyboard_read_byte(uint8_t *byte)
{
    uint32_t value;

    /*
     * Read exactly once: reading this register pops the FIFO.
     * Validity and data must come from the same transaction.
     */
    value = IORD_32DIRECT(IO_BASE, DOOM_KEY_OFFSET);

    if (!(value & DOOM_KEY_VALID)) {
        return 0;
    }

    *byte = (uint8_t)(value & DOOM_KEY_BYTE_MASK);
    return 1;
}

uint32_t keyboard_take_errors(void)
{
    uint32_t errors;

    errors = IORD_32DIRECT(IO_BASE, DOOM_ERRORS_OFFSET);
    errors &= DOOM_KEY_ERROR_OVERFLOW | DOOM_KEY_ERROR_FRAME;

    if (errors != 0u) {
        /* Error flags are write-one-to-clear. */
        IOWR_32DIRECT(IO_BASE, DOOM_ERRORS_OFFSET, errors);
    }

    return errors;
}