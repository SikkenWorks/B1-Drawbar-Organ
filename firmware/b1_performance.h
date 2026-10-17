/* GPL-3.0-only — SikkenWorks B1 V0.2 physical-key performance routing. */
#ifndef B1_PERFORMANCE_H
#define B1_PERFORMANCE_H
#include <stdint.h>
#include "b1_engine.h"
#define B1_KEYS 27u
#define B1_KEYBOARD_LOW_MIDI 53u  /* FM1 physical key 0 = F3 */
#define B1_RANGES 4u
#define B1_MAX_CHORD_NOTES 6u
#define B1_PROJECT_SLOTS 12u
#define B1_CHORD_TYPES 14u
#define B1_SCALE_COUNT 48u
enum b1_range_type { B1_ORGAN=0, B1_CHORD=1, B1_DISABLED=255 };
enum b1_chord_type { B1_MAJ=0, B1_MIN, B1_MAJ7, B1_MIN7, B1_DOM7, B1_DIM, B1_DIM7, B1_AUG, B1_SUS2, B1_SUS4, B1_ADD9, B1_MIN_ADD9, B1_MIN7B5, B1_MAJ9 };
struct b1_range { uint8_t lo,hi,type,bank;int8_t octave;uint8_t attack; };
struct b1_held {uint8_t count,note[B1_MAX_CHORD_NOTES],bank[B1_MAX_CHORD_NOTES],pending[B1_MAX_CHORD_NOTES];uint32_t due[B1_MAX_CHORD_NOTES];};
struct b1_performance {
 struct b1_range range[B1_RANGES];
 struct b1_held held[B1_KEYS];
 uint8_t chord_map[B1_KEYS];
 uint32_t touch[B1_KEYS],order;
 uint8_t global_octave,scale_root,scale_type,keylock,guide,selected_bank,chord_mode;
 /* 0=legacy auto, 1=OFF, 2=ON; never enable a selector without a range. */
 uint32_t pressed,clock,random_state;
 /* Runtime only: chord events pending, NOT part of project storage. */
 uint32_t pending_mask;
};
struct b1_project {
 uint32_t magic,version;
 uint8_t drawbars[B1_BANKS][B1_HARMONICS];
 struct b1_range ranges[B1_RANGES];
 uint8_t chords[B1_KEYS];
 uint8_t scale_root,scale_type,keylock,guide,rotor_fast,rotor_on,drive,click,leakage,percussion_on,percussion_harmonic,cv_mode,cv_depth,rotor_width,rotor_mix;
 uint8_t sound_mode,amp_gain,amp_tone,cabinet_mix;
 /* Occupies the 3 existing V5 checksum-covered padding bytes (105..107).
    sizeof(b1_project) and all flash archive offsets remain unchanged. */
 uint8_t rotor_slow_pct,rotor_fast_pct,rotor_drum_pct;
 /* V6 adds all FX settings and live performance setup. */
 uint8_t delay_mix,delay_time,delay_feedback,delay_spread;
 uint8_t reverb_type,reverb_mix,reverb_decay,reverb_damp;
 uint8_t global_octave,selected_bank,chord_mode; /* previously zeroed V6 padding byte */
 uint32_t checksum;
};
void b1_performance_init(struct b1_performance *p);
uint8_t b1_note_allowed(const struct b1_performance *p,uint8_t note);
uint8_t b1_key_allowed(const struct b1_performance *p,uint8_t physical_key);
/* Guide uses physical note pitches, NOT range/chord roles or octave assignments. */
uint8_t b1_guide_key_allowed(const struct b1_performance *p,uint8_t physical_key);
uint8_t b1_quantize_note(const struct b1_performance *p,uint8_t note);
void b1_performance_keys(struct b1_performance *p,struct b1_engine *e,uint32_t keys);
/* Sample-accurate scheduled chord note-ons, called by the FM1 DMA audio callback. */
void b1_performance_render(struct b1_performance *p,struct b1_engine *e,int16_t *out,uint32_t frames);
void b1_release_all(struct b1_performance *p,struct b1_engine *e);
/* Actual physical note-to-CHORD assignment and octave control; host-testable. */
uint8_t b1_assign_chord_key(struct b1_performance *p,uint8_t physical_key,uint8_t chord_quality);
uint8_t b1_chord_enabled(const struct b1_performance *p);
uint8_t b1_set_chord_enabled(struct b1_performance *p,struct b1_engine *e,uint8_t on);
uint8_t b1_chord_attack(const struct b1_performance *p);
uint8_t b1_set_chord_attack(struct b1_performance *p,uint8_t value);
uint8_t b1_shift_octave(struct b1_performance *p,struct b1_engine *e,int8_t change,uint32_t held_keys);
uint8_t b1_active_chord(const struct b1_performance *p);
int b1_project_valid(const struct b1_project *pr);
void b1_project_capture(struct b1_project *pr,const struct b1_performance *p,const struct b1_engine *e);
int b1_project_apply(const struct b1_project *pr,struct b1_performance *p,struct b1_engine *e);
#endif
