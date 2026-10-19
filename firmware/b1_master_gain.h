/* SPDX-License-Identifier: GPL-3.0-only
 * SikkenWorks B1 V3.7 — exact constant-denominator gain for 16-bit PCM.
 * Replaces generic signed /32767 on every L/R sample without changing one
 * output integer (C99 signed division truncates toward zero).
 * Preconditions: sample is a signed 16-bit PCM value and gain <= 32767.
 */
#ifndef B1_MASTER_GAIN_H
#define B1_MASTER_GAIN_H
#include <stdint.h>
static inline int32_t b1_master_gain_exact(int16_t sample, uint16_t gain)
{
    int32_t product = (int32_t)sample * (int32_t)gain;
    uint32_t magnitude = (uint32_t)(product < 0 ? -product : product);
    /* 32768 = 32767+1. For the bounded PCM product, the correction
     * from /32768 to /32767 can only be zero or one. */
    uint32_t quotient = magnitude >> 15;
    uint32_t rem = magnitude & 0x7fffu;
    quotient += (uint32_t)(rem + quotient >= 32767u);
    return product < 0 ? -(int32_t)quotient : (int32_t)quotient;
}
#endif
