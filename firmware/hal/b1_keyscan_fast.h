/* SikkenWorks B1 V4.3: lossless fast path for the FM1 keyboard diode matrix.
 * The 10kHz timer, 11-column sequencing, physical GPIO transitions,
 * LED PWM/breathing and the original asymmetric debounce state machine stay
 * unchanged.  GPL-3.0-only. */
#pragma once
#include <stdint.h>

/* Only bits corresponding to real keys or buttons matter for debouncing.
 * The top encoder and phantom row bits remain available for encoder logic. */
static const uint8_t b1_key_row_mask[11] = {
    0x1eu,0x1eu,0x1eu,0x1eu,0x1eu,0x1eu,0x1eu,0x1eu,
    0x1cu,0x1cu,0x1cu
};
/* Identical bytes to 0xFFFF ^ (1<<n) ^ (n<2 ? 1<<(11+n) : 0).
 * This table only removes integer shifts/xors from the time-critical ISR;
 * the bit-bang clock timing and GPIO writes remain in the original HAL. */
static const uint16_t b1_column_shift_word[11] = {
    0xf7feu,0xeffdu,0xfffbu,0xfff7u,0xffefu,0xffdfu,
    0xffbfu,0xff7fu,0xfeffu,0xfdffu,0xfbffu
};
/* Pending is nonzero until every key counter on that column returns to zero.
 * A fully settled column can be skipped only when relevant physical inputs
 * match the previous scan. As soon as raw contact changes, process this tick. */
static inline uint8_t b1_keyscan_needs_processing(uint8_t now, uint8_t prev,
                                                   uint8_t pending, uint8_t mask) {
    return (uint8_t)((((uint32_t)(now ^ prev) & mask) != 0u) || pending);
}
