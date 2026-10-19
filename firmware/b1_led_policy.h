/* SPDX-License-Identifier: GPL-3.0-only
 * FM-1 LED address translation and page/guide policy for SikkenWorks B1.
 * Hardware key IDs 0..13 = FUNCTION BUTTONS; 14..40 = 27 NOTE KEYS.
 */
#ifndef B1_LED_POLICY_H
#define B1_LED_POLICY_H
#include <stdint.h>
#include "b1_performance.h"
#define B1_FM1_BUTTON_LED_COUNT 14u
#define B1_FM1_NOTE_LED_OFFSET 14u
/* The physical button opening the current page is the only mode indicator.
 * This is intentionally independent from SCALE GUIDE and musical pitches. */
static uint8_t b1_led_page_button(uint8_t page){
 switch(page){
  case 0: return 8u; /* HOME / DRAWBARS */
  case 1: return 2u; /* FX */
  case 2: return 4u; /* ENV / PERCUSSION */
  case 3: return 6u; /* EDIT / RANGES */
  case 4: return 9u; /* SAVE / PROJECTS */
  case 5: return 7u; /* GLO / SCALE */
  case 6: return 5u; /* LFO / CHORD */
  default:return 255u;
 }
}
/* Same callback on device and in unit tests, no hardware coupling. */
static void b1_apply_leds(const struct b1_performance *p,uint32_t physical_notes,
                          uint8_t current_page,void (*set_led)(uint32_t,int)){
 uint8_t page_led=b1_led_page_button(current_page);
 for(uint32_t btn=0;btn<B1_FM1_BUTTON_LED_COUNT;btn++){
  set_led(btn,btn==page_led);
 }
 /* CHORD ASSIGN mode owns the note LEDs on performance pages: persistent
  * light = active chord selector, dark = ordinary ORGAN/DISABLED key.
  * SCALE is an explicit exception: it keeps its dedicated 48-scale GUIDE.
  * While on the CHORD editor with ASSIGN OFF, all idle note LEDs are dark.
  * Physical key-down feedback is preserved in every mode. */
 uint8_t chord_display=(uint8_t)(current_page!=5u && b1_chord_enabled(p));
 for(uint32_t key=0;key<B1_KEYS;key++){
  int pressed=(physical_notes&(1u<<key))!=0u;
  int led_on;
  if(chord_display)led_on=b1_chord_selector_key(p,(uint8_t)key);
  else if(current_page==6u)led_on=0;
  else led_on=p->guide && b1_guide_key_allowed(p,(uint8_t)key);
  set_led(B1_FM1_NOTE_LED_OFFSET+key,pressed || led_on);
 }
}
#endif
