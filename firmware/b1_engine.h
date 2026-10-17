/* SPDX-License-Identifier: GPL-3.0-only
 * SikkenWorks B1 V0.9. Original clonewheel DSP, no proprietary samples.
 * Realtime-only fixed point code; 44.1 kHz stereo.
 */
#ifndef B1_ENGINE_H
#define B1_ENGINE_H
#include <stdint.h>
#define B1_SAMPLE_RATE 44100u
#define B1_VOICES 12u
#define B1_HARMONICS 9u
#define B1_BANKS 3u
#define B1_TONEWHEELS 91u
#define B1_ROTARY_SAMPLES 1024u
#define B1_CV_SAMPLES 512u
#define B1_DELAY_SAMPLES 16384u
#define B1_REVERB_A 1024u
#define B1_REVERB_B 2048u
#define B1_REVERB_C 4096u
#define B1_REVERB_DIFF 512u
#define B1_SOUND_B3 0u
#define B1_SOUND_ROCK 1u
struct b1_voice {
  uint32_t seed,owner;
  uint16_t envelope,age,release_click_age;
  uint8_t wheel[B1_HARMONICS];
  uint8_t note,bank,velocity,active,releasing;
};
struct b1_engine {
  struct b1_voice voice[B1_VOICES];
  uint8_t drawbars[B1_BANKS][B1_HARMONICS];
  uint32_t pull_smooth_q16[B1_BANKS][B1_HARMONICS];
  /* Per-sample memoized tonewheel output. Shared frequencies must be evaluated
     only once per audio frame even when many keys use the same wheel. This
     is fixed-size storage, never heap allocation in the realtime callback. */
  int32_t wheel_value[B1_TONEWHEELS];
  uint32_t wheel_stamp[B1_TONEWHEELS];
  uint32_t wheel_frame, wheel_last_phase_frame;
  uint32_t wheel_phase_stamp[B1_TONEWHEELS]; /* phase update only for actually used wheels */
  /* Exactly 91 shared free-running tonewheels: notes reuse phase, never retrigger oscillators. */
  uint32_t wheel_phase[B1_TONEWHEELS];
  uint32_t wheel_inc[B1_TONEWHEELS];
  uint32_t rotor_phase,rotor_inc;
  uint32_t drum_phase,drum_inc;
  uint8_t rotor_fast,rotor_enabled;
  /* 0 = legacy/factory 100%; 50..150 is user-adjustable percent. */
  uint8_t rotor_slow_pct,rotor_fast_pct,rotor_drum_pct;
  uint16_t registration_gain_q15;
  uint8_t percussion_enabled,percussion_harmonic,drive,click,leakage;
  uint8_t cv_mode,cv_depth,rotor_width,rotor_mix;
  uint8_t sound_mode,amp_gain,amp_tone,cabinet_mix;
  uint8_t delay_mix,delay_time,delay_feedback,delay_spread;
  uint8_t reverb_type,reverb_mix,reverb_decay,reverb_damp;
  uint16_t cv_wet_q15;
  uint32_t fx_tail_samples,fx_clear_step;
  uint8_t fx_was_audible;
  uint8_t cpu_relief; /* Auto-fallback if real FM1 DMA misses audio deadlines. */
  /* Key-gated output: never expose unquantized filter/delay residue when idle. */
  uint16_t output_gate_q15;
  uint8_t fx_cleared;
  uint16_t mode_mix_q15;
  uint8_t upper_held,percussion_latched,percussion_wheel;
  uint32_t percussion_env;
  uint32_t cv_phase,cv_write,rotor_write;
  int32_t cabinet_hp,rotor_lp1,rotor_lp2;
  int32_t amp_hp,amp_tone_lp,cab_bass,cab_treble,cab_presence,amp_sag;
  /* Preallocated audio memories; no malloc, flash writes or locks in ISR. */
  uint32_t delay_write,rev_write;
  int32_t rev_damp_a,rev_damp_b,rev_damp_c;
  int16_t delay_line[B1_DELAY_SAMPLES];
  int16_t rev_a[B1_REVERB_A],rev_b[B1_REVERB_B],rev_c[B1_REVERB_C],rev_diff[B1_REVERB_DIFF];
  int16_t cv_line[B1_CV_SAMPLES];
  int16_t rotor_line[B1_ROTARY_SAMPLES]; /* treble horn path */
  int16_t drum_line[B1_ROTARY_SAMPLES];  /* bass drum path */
};
void b1_reset(struct b1_engine *e);
void b1_drawbar(struct b1_engine *e,uint8_t bank,uint8_t bar,uint8_t position);
void b1_note_on(struct b1_engine *e,uint8_t midi_note,uint8_t bank,uint8_t velocity);
void b1_note_on_tagged(struct b1_engine *e,uint8_t midi_note,uint8_t bank,uint8_t velocity,uint32_t owner);
void b1_note_off_tagged(struct b1_engine *e,uint32_t owner);
void b1_note_off(struct b1_engine *e,uint8_t midi_note,uint8_t bank);
void b1_panic(struct b1_engine *e);
void b1_render_stereo(struct b1_engine *e,int16_t *interleaved,uint32_t frames);
/* Tonewheel map deliberately available to regression tests / future calibration. */
uint8_t b1_tonewheel_index(uint8_t midi_note,uint8_t footage);
#endif
