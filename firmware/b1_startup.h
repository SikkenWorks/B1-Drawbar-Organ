/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef B1_STARTUP_H
#define B1_STARTUP_H
#include <stdint.h>
#include "b1_ui.h"
#define B1_STARTUP_DURATION_MS 3220u
#define B1_STARTUP_HOLD_BEGIN_MS 1300u
#define B1_STARTUP_HOLD_END_MS 2900u
/* Both firmware and exact-frame tests call these identical C functions. */
void b1_startup_render(uint16_t *fb,const struct b1_ui_state *home,uint32_t ms);
void b1_startup_leds(uint32_t ms,void (*set_led)(uint32_t,int));
#endif
