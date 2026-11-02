#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* Returns 1 and consumes one byte, or returns 0 when empty. */
int keyboard_read_byte(uint8_t *byte);

/* Returns and clears the currently reported error flags. */
uint32_t keyboard_take_errors(void);

#endif // KEYBOARD_H