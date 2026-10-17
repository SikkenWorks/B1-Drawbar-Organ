/* SPDX-License-Identifier: GPL-3.0-only. */
#ifndef B1_UI_H
#define B1_UI_H
#include <stdint.h>
#include "b1_performance.h"
#include "b1_storage.h"
struct b1_ui_state {
 const struct b1_engine *audio;
 const struct b1_performance *keys;
 uint8_t page,drawbar_page,fx_page,safe_mode,overrun;
 uint8_t selected_range,chord_cursor,chord_quality,global_octave,project_cursor,project_new_armed;
 uint16_t project_mask;
 const char (*project_names)[B1_PROJECT_NAME_MAX+1u];
 const char *edit_name;
 uint8_t name_cursor,project_storage_state,dirty_project;
 int8_t project_error;
 uint8_t audio_diag,audio_test,audio_safe;
 uint32_t audio_blocks,audio_misses;
 uint8_t ota_step; int32_t ota_error;
 uint8_t reclaim_armed;
 uint8_t project_notice;
};
/* Frame must contain 240 * 240 uint16_t pixels. Exactly the same function
 * is linked into the real FM1 firmware and into native screenshot tests. */
const char *b1_root_name(uint8_t root);
const char *b1_scale_name(uint8_t scale);
void b1_ui_render(uint16_t *frame,const struct b1_ui_state *state);
#endif
