/* SikkenWorks B1 V4.2: cooperative main-thread idle for JieLi pi32v2.
 * No timers/interrupts are masked. AUDIO DMA, TIMER5 LED/key scan and USB
 * remain interrupt-driven; the next interrupt resumes execution.
 * No power-domain/clock gating or DSP state changes. GPL-3.0-only.
 */
#pragma once
#include <stdint.h>

/* Pure scheduling guard: all urgent work takes precedence over CPU idle.
 * Caller performs housekeeping at least once per b1_ms tick (1 ms),
 * including USB reconnect and OTA identity-service checks.
 */
static inline uint8_t b1_can_idle_main(uint32_t now_ms, uint32_t last_ms,
                                      uint8_t boot_req, uint8_t ota_req,
                                      uint8_t overrun){
 return (uint8_t)(now_ms==last_ms && !boot_req && !ota_req && !overrun);
}

static inline void b1_cpu_wait_for_irq(void){
#if defined(__pi32v2__) || defined(__PI32V2__) || defined(B1_TARGET_PI32V2)
 #if !defined(B1_NO_CPU_IDLE)
  /* pi32v2 IDLE opcode 0x0001. Hardware wakes on an enabled interrupt.
   * TIMER5 is enabled at 10kHz, so this is not a deep-sleep state. */
  __asm__ volatile("idle" ::: "memory");
 #else
  __asm__ volatile("" ::: "memory");
 #endif
#else
 /* Host C tests exercise scheduling decisions; never execute pi32 opcode. */
 __asm__ volatile("" ::: "memory");
#endif
}
