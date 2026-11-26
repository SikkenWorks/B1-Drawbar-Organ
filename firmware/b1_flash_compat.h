/* SPDX-License-Identifier: GPL-3.0-only
 * B1 V5.7: exact, positive hardware allowlist. Unknown SPI NOR must not be
 * erased even if the generic SPI opcodes happen to respond.
 * Read the ID with fl_jedec_ram() while interrupts are disabled, outside audio.
 */
#ifndef B1_FLASH_COMPAT_H
#define B1_FLASH_COMPAT_H
#include <stdint.h>
#define B1_FLASH_PUYA_P25Q80H 0x856014u
#define B1_FLASH_UCUN_UC25HQ80 0xB36014u
static inline uint8_t b1_flash_supported_id(uint32_t jedec)
{
    return (uint8_t)(jedec == B1_FLASH_PUYA_P25Q80H ||
                     jedec == B1_FLASH_UCUN_UC25HQ80);
}
#endif
