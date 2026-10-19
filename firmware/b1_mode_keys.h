/* SPDX-License-Identifier: GPL-3.0-only
 * HOME + FX chord recognizer for FM1 debounced physical buttons.
 * Defer standalone HOME/FX for 80ms so pressing them slightly apart
 * doesn't panic the running notes or navigate to FX. No ISR allocations. */
#ifndef B1_MODE_KEYS_H
#define B1_MODE_KEYS_H
#include <stdint.h>
#define B1_MODE_HOME (1u<<8)
#define B1_MODE_FX (1u<<2)
#define B1_MODE_COMBO (B1_MODE_HOME|B1_MODE_FX)
#define B1_MODE_GRACE_MS 80u
struct b1_mode_keys_state { uint32_t pending,since; uint8_t latched; };
static uint32_t b1_mode_keys_filter(struct b1_mode_keys_state *s,
 uint32_t pressed,uint32_t held,uint32_t now,uint8_t *toggle){
 *toggle=0u;
 if((held&B1_MODE_COMBO)==B1_MODE_COMBO){
  if(!s->latched){s->latched=1u;*toggle=1u;}
  s->pending=0u;
  return pressed&~B1_MODE_COMBO;
 }
 s->latched=0u;
 uint32_t fresh=pressed&B1_MODE_COMBO;
 if(fresh){
  /* Only remember first edge; second button combined is handled above. */
  if(!s->pending)s->since=now;
  s->pending|=fresh;
  pressed&=~B1_MODE_COMBO;
 }
 if(s->pending && (!(held&s->pending) || (uint32_t)(now-s->since)>=B1_MODE_GRACE_MS)){
  pressed|=s->pending;
  s->pending=0u;
 }
 return pressed;
}
#endif
