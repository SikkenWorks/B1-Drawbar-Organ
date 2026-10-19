/* SPDX-License-Identifier: GPL-3.0-only
 * SikkenWorks B1 V3.7 LCD damage detection. Does not depend on hardware.
 * The RGB565 framebuffer is still drawn in full, exactly as V3.4; only
 * unchanged rows are omitted from SPI transfer. Fail-safe full refresh
 * every 64 presentations limits the lifetime of an unlikely hash collision.
 */
#ifndef B1_LCD_DIRTY_H
#define B1_LCD_DIRTY_H
#include <stdint.h>
#define B1_LCD_WIDTH 240u
#define B1_LCD_HEIGHT 240u
#define B1_LCD_MAX_STRIP 115u
#define B1_LCD_MERGE_GAP 2u
#define B1_LCD_FULL_INTERVAL 64u
struct b1_lcd_damage {
    uint32_t hashes[B1_LCD_HEIGHT];
    uint32_t presentations;
    uint8_t valid;
};
/* Wordwise FNV-1a 32-bit fingerprint. RGB565 is already a 16-bit pixel,
 * so hashing it directly eliminates one XOR and one multiplication per pixel
 * versus the bytewise V3.6 hash. This is byte-order-neutral. Each row is
 * periodically sent in full regardless of its fingerprint (64 frames).
 * Fingerprints are hints: a rare collision can delay a visual update, never
 * corrupt audio, flash or a persistent project. */
static uint32_t b1_lcd_row_hash(const uint16_t *row)
{
    uint32_t hash = 2166136261u;
    for (uint32_t x = 0; x < B1_LCD_WIDTH; ++x) {
        uint16_t pixel = row[x];
        hash = (hash ^ (uint32_t)pixel) * 16777619u;
    }
    return hash;
}
/* dirty[] has 240 entries. Returns count of changed rows. Updates the cache
 * only when the caller will actually present the resulting framebuffer. */
static uint32_t b1_lcd_detect_damage(struct b1_lcd_damage *state,
                                     const uint16_t *frame,
                                     uint8_t dirty[B1_LCD_HEIGHT])
{
    uint32_t count = 0;
    uint8_t full = (uint8_t)(!state->valid ||
                      (state->presentations % B1_LCD_FULL_INTERVAL) == 0u);
    for (uint32_t y = 0; y < B1_LCD_HEIGHT; ++y) {
        uint32_t hash = b1_lcd_row_hash(frame + y * B1_LCD_WIDTH);
        uint8_t change = (uint8_t)(full || hash != state->hashes[y]);
        dirty[y] = change;
        state->hashes[y] = hash;
        count += change;
    }
    state->valid = 1u;
    ++state->presentations;
    return count;
}
/* Find next bounded contiguous/near-contiguous range. Sparse changes can
 * absorb up to two clean rows to avoid repeated SPI window commands. */
static uint8_t b1_lcd_next_damage(const uint8_t dirty[B1_LCD_HEIGHT],
                                 uint32_t *cursor,
                                 uint32_t *start, uint32_t *count)
{
    uint32_t y = *cursor;
    while (y < B1_LCD_HEIGHT && !dirty[y]) ++y;
    if (y == B1_LCD_HEIGHT) { *cursor = y; return 0u; }
    *start = y;
    uint32_t last = y;
    for (++y; y < B1_LCD_HEIGHT && y - *start < B1_LCD_MAX_STRIP; ++y) {
        if (dirty[y]) last = y;
        if (y - last > B1_LCD_MERGE_GAP) break;
    }
    *count = last - *start + 1u;
    *cursor = last + 1u;
    return 1u;
}
#endif
