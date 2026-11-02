#ifndef INCLUDE_PS2_KEYS_H_
#define INCLUDE_PS2_KEYS_H_

#include <stdint.h>

typedef struct {
    uint8_t extended;
    uint8_t released;
    uint8_t pause_skip;
} Ps2Parser;

/* Returns 1 when a recognised key event is ready; otherwise 0. */
int ps2_key(
    Ps2Parser *parser,
    uint8_t byte,
    int *pressed,
    uint8_t *key
);

#endif /* INCLUDE_PS2_KEYS_H_ */
