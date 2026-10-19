/* V4.3 deterministic main-loop scheduling tests: GPL-3.0-only. */
#include <stdio.h>
#include <stdint.h>
#include "b1_cpu_idle.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
  uint32_t run=0,idle=0,last=UINT32_MAX;
  for(uint32_t tick=0;tick<100000u;tick++){
    uint32_t now=tick/10u; /* TIMER5 10kHz => b1_ms 1kHz */
    if(b1_can_idle_main(now,last,0u,0u,0u)){++idle;continue;}
    ++run;last=now;
  }
  CHECK(run==10000u);CHECK(idle==90000u);
  CHECK(b1_can_idle_main(120u,120u,0u,0u,0u));
  CHECK(!b1_can_idle_main(120u,120u,1u,0u,0u)); /* urgent UBOOT */
  CHECK(!b1_can_idle_main(120u,120u,0u,1u,0u)); /* urgent OTA */
  CHECK(!b1_can_idle_main(120u,120u,0u,0u,1u)); /* urgent DMA */
  CHECK(!b1_can_idle_main(0u,UINT32_MAX,0u,0u,0u)); /* millis wrap */
  CHECK(!b1_can_idle_main(1u,0u,0u,0u,0u));
  b1_cpu_wait_for_irq(); /* host stub, does not sleep */
  printf("PASS V4.3: 10k foreground ticks -> 1k housekeeping calls, 9k IRQ-idle opportunities per second; urgent flags unthrottled\n");
  return 0;
}
