/* SikkenWorks B1 V4.7 (Cross-firmware native OTA and lossless polyphony; V4.5 DSP + Smart Idle + KEY ASSIGN intact). GPL-3.0-only, custom FM1 executable.
 * Felucca platform HAL © Leo Kuroshita / Hügelton Instruments.
 * FLASH POLICY: OTA staging plus B1-owned, dynamically selected verified-safe two-sector journal.
 * BEETLE project data at 0x97000..0xDFFFF is never modified.
 * Firmware is untested on physical hardware; recovery outside the running app is unproven.
 */
#include <stdint.h>
#include "hal/fm1_cc.h"
#include "hal/fm1_time.h"
#include "hal/fm1_sys.h"
#include "hal/fm1_irq.h"
#include "hal/fm1_guard.h"
#include "hal/fm1_input.h"
#include "hal/fm1_timer.h"
#include "hal/fm1_audio.h"
#include "hal/fm1_adc.h"
#define ETCH_FLASH_READ_ONLY 0
#define ETCH_NATIVE_OTA 1
#define ETCH_NATIVE_STORAGE 1
/* Do not give B1 any write access to BEETLE or Felucca project storage. */
#define FL_RANGE_OK(off,n) (FL_IN(off,n,0x000E0000u,0x000E5000u) || FL_IN(off,n,0x000E7000u,0x000F0000u))
#include "hal/fm1_flash.h"
#include "hal/fm1_lcd_hw.h"
#include "b1_performance.h"
#include "b1_ui.h"
#include "b1_lcd_dirty.h"
#include "b1_master_gain.h"
#include "b1_startup.h"
#include "b1_led_policy.h"
#include "b1_mode_keys.h"
#include "b1_cpu_idle.h"
#include "b1_storage.h"
#include "libc.c"
#include "lcd.c"
#define FELUCCA_CDC 0
#define FELUCCA_OTA 1
#define FELUCCA_OTA_DRYRUN 0
#define FELUCCA_UAC 0
#define FELUCCA_ID "FM-1_2147"
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define HALF_FRAMES 128u
#include "usb.c"

extern uint32_t _bss_start[],_bss_end[],_pool_start[],_pool_end[];
extern uint32_t _data_load[],_data_start[],_data_end[],_rt_load[],_rt_start[],_rt_end[];
extern void isr_timer5(void),isr_alnk0(void);
static volatile uint32_t b1_ms,b1_ticks;
static volatile uint8_t b1_audio_active,b1_isr_active,b1_overrun;
static struct b1_engine b1_audio __attribute__((section(".pool")));
static struct b1_performance b1_keys __attribute__((section(".pool")));
static struct b1_storage b1_saved __attribute__((section(".pool")));
static struct b1_project_archive b1_pending_save __attribute__((section(".pool")));
static uint8_t project_cursor,dirty_project,selected_range;
static uint8_t chord_cursor,chord_quality,project_new_armed,name_cursor;
static char edit_name[B1_PROJECT_NAME_MAX+1u];
static int8_t project_error;
static uint8_t project_io_busy;
static uint8_t project_notice; /* 1=SAVE OK 2=LOAD OK, temporary */
static uint32_t project_notice_until;
static volatile uint8_t save_requested;
/* 1=load saved slot, 2=new blank project. Execution is deferred until AFTER
 * the short UI interrupt-off section, so zeroing the 56KB engine and applying
 * a saved project never blocks ALNK0 DMA or the USB clock interrupt. */
static uint8_t b1_project_transition_requested;
static uint8_t b1_transition_slot; /* snapshotted BEFORE simultaneous encoder edits */
static volatile uint32_t b1_ui_irq_peak_ticks;

static uint8_t reclaim_armed;
static uint8_t ui_page,drawbar_page,fx_page;
static struct b1_mode_keys_state b1_mode_key_state;
/* Audio diagnostics are on SCL -> SELECT (not the SCALE keyboard LEDs). */
static volatile uint8_t b1_audio_diag,b1_test_tone,b1_force_safe;
static uint8_t b1_safe_latched;
static uint16_t b1_good_dma_blocks;
static uint32_t b1_probe_phase;
/* 128 frames at 44.1 kHz = 2.90ms, timer4 ticks at 24MHz.
 * 48k ticks (~2ms) is an early-warning threshold with margin for DMA.
 * Once a real missed deadline occurs, keep the low-cost path latched for
 * this boot. Repeated full-path reactivation would create glitch loops. */
#define B1_DMA_SOFT_BUDGET_TICKS 48000u
#define B1_DMA_HARD_BUDGET_TICKS 69632u
static volatile uint8_t output_ready;
static uint8_t safe_mode;
static uint16_t fade;
static int32_t b1_dma[HALF_FRAMES*4] __attribute__((aligned(4)));
static int16_t b1_pcm[HALF_FRAMES*2] __attribute__((section(".pool")));
static uint16_t b1_frame[240u*240u] __attribute__((section(".pool"),aligned(4)));
static uint32_t b1_midi_overruns,b1_dma_deadlines;
static volatile uint32_t b1_audio_blocks,b1_audio_overruns,b1_audio_peak_ticks;
static struct b1_lcd_damage b1_lcd_cache;
static volatile uint8_t b1_cpu_relief;
static volatile uint8_t b1_ui_dirty;
static volatile uint8_t b1_updating;
static uint8_t b1_ota_step;
static int32_t b1_ota_error;
static void b1_screen(void);
static void b1_present_frame(void);
static void b1_refresh_leds(void);
#include "ota.c"
#include "b1_ota_target.c"
static volatile uint16_t b1_master_target;
static uint16_t b1_master_smoothed;

void fm1_timer5_irq(void){
 fm1_timer5_ack();fm1_input_tick();++b1_ticks;
 if(b1_ticks%10u==0u)++b1_ms;
 /* Keep USB endpoint processing out of the codec DMA-critical render. */
 /* Keep USB enumeration and the update channel alive even if DSP is busy.
  * Only TIMER5 touches SIE registers. Before USB is configured we cannot
  * skip USB polls due to an overloaded audio callback: doing so leaves the
  * FM1 invisible to installers. REC boot also skips audio entirely. */
 if(b1_ticks%5u==0u && (!b1_isr_active || !usb.config || safe_mode))usb_poll();
}
static void b1_drain_midi(void){
 uint32_t end=mi_w;
 if(midi_in_overflow){mi_r=end;midi_in_overflow=0u;b1_panic(&b1_audio);b1_release_all(&b1_keys,&b1_audio);++b1_midi_overruns;return;}
 for(uint32_t n=0;n<MQ && mi_r!=end;n++){
  uint32_t pkt=midi_in_q[mi_r%MQ];++mi_r;
  uint8_t st=(uint8_t)(pkt>>8),note=(uint8_t)(pkt>>16),vel=(uint8_t)(pkt>>24);
  uint8_t bank=(uint8_t)(st&0x0Fu);if(bank>=B1_BANKS)bank=0u;
  /* Channels 1, 2, 3 select UPPER, LOWER and SOLO independently. */
  if((st&0xF0u)==0x90u && vel)b1_note_on(&b1_audio,note,bank,vel);
  else if((st&0xF0u)==0x80u||((st&0xF0u)==0x90u&&!vel))b1_note_off(&b1_audio,note,bank);
  else if((st&0xF0u)==0xB0u && note==123u)b1_panic(&b1_audio);
 }
}
static void fm1_fault(const fm1_crash_t *c){
 (void)c;fm1_audio_output_enable(0u);fm1_audio_stop();fm1_irq_off();fm1_enter_uboot();
}
void fm1_alnk0_irq(void){
 b1_isr_active=1;
 uint8_t pending=fm1_audio_pending();fm1_audio_ack_aux(pending);
 if(pending&FM1_AUDIO_HALF){
  uint32_t half=fm1_audio_free_half();
  /* M-VAVE Open V12 acknowledges the current half event before rendering.
   * Otherwise a pending ownership check cannot distinguish the current
   * event from a *new* hardware deadline during processing. */
  fm1_audio_ack_half();
  uint32_t start=fm1_ticks();
  if(b1_audio_active){
   if(b1_test_tone){
    /* Independent low-level 440Hz triangle test, -24dBFS peak.
     * Tests ALNK0 -> codec -> master level with no oscillator/Leslie DSP.
     * Still uses the normal stereo DMA and master gain/ramp logic. */
    for(uint32_t t=0;t<HALF_FRAMES;t++){
     b1_probe_phase+=42852281u; /* 440 Hz at 44,100 Hz */
     uint32_t phase=(b1_probe_phase>>16)&65535u;
     int32_t tri=phase<32768u?(int32_t)phase:(int32_t)(65535u-phase);
     int16_t sample=(int16_t)((tri-16384)/8);
     b1_pcm[t*2u]=sample;b1_pcm[t*2u+1u]=sample;
    }
   }else{
    /* V2.8: CHORD ASSIGN is also a playable live audition page. Previously
     * every key was forced OFF here, hiding the humanized ATTACK entirely.
     * The editor only changes assignments for selector-range key edges in
     * the UI loop; ordinary ORGAN keys continue through the same scheduler. */
    uint32_t keys=fm1_in.notes;
    if(keys!=b1_keys.pressed)b1_performance_keys(&b1_keys,&b1_audio,keys);
    if(mi_r!=mi_w || midi_in_overflow)b1_drain_midi();
    b1_performance_render(&b1_keys,&b1_audio,b1_pcm,HALF_FRAMES);
   }
   /* Buffer-ownership check. Drop the block rather than touch a live DMA half. */
   uint32_t cost=(uint32_t)(fm1_ticks()-start);
   if(!fm1_audio_half_owned(half) || cost>B1_DMA_HARD_BUDGET_TICKS){
    /* On overrun, immediately fall back to bounded DSP, and NEVER submit
     * a corrupted partial PCM render. An ALNK free half may only be written
     * while it is still FREE. If it is free, replace stale samples with
     * silence. Otherwise leave the DMA-owned half untouched. Audio resumes
     * with the next safe callback; no stale PCM is deliberately reissued. */
    b1_overrun=1u;++b1_audio_overruns;++b1_dma_deadlines;
    b1_safe_latched=1u;b1_audio.cpu_relief=1u;
    if(fm1_audio_half_owned(half)){
     int32_t *silence=&b1_dma[half*HALF_FRAMES*2u];
     for(uint32_t i=0;i<HALF_FRAMES*2u;i++)silence[i]=0;
    }
    if(cost>b1_audio_peak_ticks)b1_audio_peak_ticks=cost;
    b1_isr_active=0;return;
   }
   if(cost>B1_DMA_SOFT_BUDGET_TICKS){
    /* Keep audio output intact, but preempt another imminent failure. */
    b1_safe_latched=1u;b1_audio.cpu_relief=1u;
   }else if(b1_good_dma_blocks<65000u){++b1_good_dma_blocks;}
   /* Commit is bounded and only to the currently free DMA half. The
    * second ownership check makes missed deadlines observable instead of
    * silently accepting a partially completed write. */
   if(!fm1_audio_half_owned(half)){
    b1_overrun=1u;++b1_audio_overruns;++b1_dma_deadlines;
    b1_safe_latched=1u;b1_audio.cpu_relief=1u;
    if(cost>b1_audio_peak_ticks)b1_audio_peak_ticks=cost;
    b1_isr_active=0;return;
   }
   int32_t *dst=&b1_dma[half*HALF_FRAMES*2u];
   for(uint32_t i=0;i<HALF_FRAMES*2u;i++){
    int32_t y=b1_pcm[i];
    if(!output_ready){y=0;dst[i]=0;continue;}
    if((i&1u)==0u){
     int32_t diff=(int32_t)b1_master_target-(int32_t)b1_master_smoothed;
     if(diff) b1_master_smoothed=(uint16_t)((int32_t)b1_master_smoothed+(diff>0?(diff+255)/256:-((-diff+255)/256)));
    }
    /* V3.7: exact 16-bit fixed-domain divide replacement. Tests cover
     * every signed PCM value at edge gains + randomized combinations. */
    y=b1_master_gain_exact((int16_t)y,b1_master_smoothed);
    if(fade<4096u){y=(y*(int32_t)fade)/4096; if((i&1u)&&fade<4096u)++fade;}
    dst[i]=y*128;
   }
   __asm__ volatile("csync" ::: "memory");
   if(!fm1_audio_half_owned(half)){
    b1_overrun=1u;++b1_audio_overruns;++b1_dma_deadlines;
    b1_safe_latched=1u;b1_audio.cpu_relief=1u;
   }
   uint32_t elapsed=(uint32_t)(fm1_ticks()-start);
   if(elapsed>b1_audio_peak_ticks)b1_audio_peak_ticks=elapsed;
  }else{
   /* V3.6: a pending project transaction pauses only synthesis, not DMA or
    * timer/USB interrupts. Fill the CPU-owned half with silence while the
    * main loop resets/applies the large engine state with IRQs enabled.
    * Avoid touching any half after its new deadline has arrived. */
   if(fm1_audio_half_owned(half)){
    int32_t *dst=&b1_dma[half*HALF_FRAMES*2u];
    for(uint32_t i=0;i<HALF_FRAMES*2u;i++)dst[i]=0;
    __asm__ volatile("csync" ::: "memory");
    if(!fm1_audio_half_owned(half)){
     b1_overrun=1u;++b1_audio_overruns;++b1_dma_deadlines;
    }
   }else{
    b1_overrun=1u;++b1_audio_overruns;++b1_dma_deadlines;
   }
  }
  ++b1_audio_blocks;
 }
 b1_isr_active=0;
}
static void b1_screen(void){
 const struct b1_ui_state ui={
  .audio=&b1_audio,.keys=&b1_keys,.page=ui_page,.drawbar_page=drawbar_page,
  .fx_page=fx_page,.safe_mode=safe_mode,.overrun=(uint8_t)(b1_overrun||b1_cpu_relief),
  .ota_step=b1_ota_step,.ota_error=b1_ota_error,
  .audio_diag=b1_audio_diag,.audio_test=b1_test_tone,
  .audio_safe=(uint8_t)(b1_force_safe||b1_safe_latched||b1_audio.cpu_relief),
  .audio_blocks=b1_audio_blocks,.audio_misses=b1_audio_overruns,
  .audio_peak_us=(b1_audio_peak_ticks+12u)/24u,
  .ui_irq_peak_us=(b1_ui_irq_peak_ticks+12u)/24u,
  .selected_range=selected_range,.chord_cursor=chord_cursor,
  .global_octave=b1_keys.global_octave,.chord_quality=chord_quality,
  .project_cursor=project_cursor,.project_mask=(uint16_t)b1_saved.archive.mask,
  .project_new_armed=project_new_armed,.project_names=b1_saved.archive.names,
  .project_storage_state=b1_saved.state,.project_error=project_error,
  .name_cursor=name_cursor,.edit_name=edit_name,.dirty_project=dirty_project,
  .reclaim_armed=reclaim_armed,
  .project_notice=(uint8_t)((int32_t)(project_notice_until-b1_ms)>0?project_notice:0u)
 };
 b1_ui_render(b1_frame,&ui);
 b1_present_frame();
}
/* LCD never sees a partially composed frame. Screen and particle renderer
 * both use this identical row-bounded transfer path. */
static void b1_present_frame(void){
 uint8_t dirty[B1_LCD_HEIGHT];
 if(!b1_lcd_detect_damage(&b1_lcd_cache,b1_frame,dirty))return;
 uint32_t cursor=0u,y,rows;
 while(b1_lcd_next_damage(dirty,&cursor,&y,&rows)){
  lcd_window(0u,y,B1_LCD_WIDTH-1u,y+rows-1u);
  lcd_data(&b1_frame[y*B1_LCD_WIDTH],B1_LCD_WIDTH*rows*2u);
  lcd_sync(); /* Frame can be safely rewritten only after DMA has finished. */
 }
}
static void b1_startup_animation(void){
 const struct b1_ui_state home={.audio=&b1_audio,.keys=&b1_keys,
  .page=0u,.drawbar_page=0u,.fx_page=0u};
 uint32_t begun=b1_ms,last_frame=0xffffffffu;
 while((b1_ms-begun)<B1_STARTUP_DURATION_MS){
  fm1_wdt_feed();usb_retry(b1_ms);
  if(usb.uboot_req||usb.ota_req)break;
  /* Performance bypass: HOME or a musical key skips the intro. The two
   * recovery boot shortcuts are checked before this routine starts. */
  if((fm1_in.buttons&(1u<<8))||fm1_in.notes)break;
  uint32_t elapsed=b1_ms-begun;
  if(last_frame!=0xffffffffu && elapsed-last_frame<55u)continue;
  last_frame=elapsed;
  b1_startup_render(b1_frame,&home,elapsed);
  b1_present_frame();
  b1_startup_leds(elapsed,fm1_led_key);
 }
 /* Restore the regular LED policy before the playable synth appears. */
 b1_refresh_leds();
}
/* B1-private storage: only sectors positively verified blank or B1-owned.
 * Each write is performed while hardware audio is stopped. ISR never writes.
 * BEETLE's 12-project partition at 0x97000..0xDFFFF is untouchable. */
static int b1_io_read(void *ctx,uint32_t addr,uint8_t *buf,uint32_t n){
 (void)ctx;
 if(!FL_IN(addr,n,B1_STORAGE_SCAN_START,B1_STORAGE_SCAN_END))return -1;
 while(n){uint32_t count=n>256u?256u:n;
  fm1_irq_off();int rc=FL_FAR(fl_read_ram)(addr,buf,count);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return rc;addr+=count;buf+=count;n-=count;
 }
 return 0;
}
static int b1_io_erase(void *ctx,uint32_t addr){
 (void)ctx;uint32_t took=0u;
 if(!FL_IN(addr,B1_STORAGE_SECTOR,B1_STORAGE_SCAN_START,B1_STORAGE_SCAN_END) || (addr & (B1_STORAGE_SECTOR-1u)))return -1;
 fm1_irq_off();int rc=FL_FAR(fl_erase4k_ram)(addr,&took);fm1_irq_enable_all();
 fm1_wdt_feed();return rc;
}
static int b1_io_program(void *ctx,uint32_t addr,const uint8_t *buf,uint32_t n){
 (void)ctx;
 if(!n||!FL_IN(addr,n,B1_STORAGE_SCAN_START,B1_STORAGE_SCAN_END))return -1;
 while(n){uint32_t count=256u-(addr&255u);if(count>n)count=n;
  uint32_t took=0u;fm1_irq_off();int rc=FL_FAR(fl_prog_ram)(addr,buf,count,&took);fm1_irq_enable_all();
  fm1_wdt_feed();if(rc)return rc;addr+=count;buf+=count;n-=count;
 }
 return 0;
}
static const struct b1_storage_io b1_flash_io={0,b1_io_read,b1_io_erase,b1_io_program};
static void b1_stop_for_write(void){
 fm1_audio_output_enable(0u);
 fm1_irq_off();b1_audio_active=0u;output_ready=0u;
 fm1_audio_stop();fm1_irq_enable_all();project_io_busy=1u;
}
static void b1_resume_after_write(void){
 for(uint32_t i=0;i<HALF_FRAMES*4u;i++)b1_dma[i]=0;
 b1_master_smoothed=0u;fade=0u;
 fm1_irq_off();fm1_audio_init(b1_dma,HALF_FRAMES*2u,isr_alnk0,3);
 b1_audio_active=1u;fm1_irq_enable_all();
 /* Analog output is enabled only after a silent DMA prime interval. */
 uint32_t until=b1_ms+330u;
 while((int32_t)(b1_ms-until)<0){fm1_wdt_feed();usb_retry(b1_ms);}
 fm1_audio_output_enable(1u);output_ready=1u;project_io_busy=0u;
}
static const char b1_name_chars[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.";
static void b1_name_edit_begin(void){
 if(b1_saved.archive.mask&(1u<<project_cursor)){
  for(unsigned i=0;i<=B1_PROJECT_NAME_MAX;i++)edit_name[i]=b1_saved.archive.names[project_cursor][i];
 }else b1_storage_default_name(project_cursor,edit_name);
 name_cursor=0u;ui_page=7u;project_error=0;
}
static void b1_name_change(int32_t delta){
 unsigned n=sizeof(b1_name_chars)-1u;
 char current=edit_name[name_cursor];unsigned i=0u;
 for(;i<n;i++)if(current==b1_name_chars[i])break;
 if(i==n)i=0u;
 int32_t next=(int32_t)i+delta;
 if(next<0)next=0;if(next>=(int32_t)n)next=(int32_t)n-1;
 edit_name[name_cursor]=b1_name_chars[next];
 /* Name editor keeps a 16-character fixed canvas. A later NUL is mandatory. */
 for(unsigned j=0;j<name_cursor;j++)if(!edit_name[j])edit_name[j]=' ';
 edit_name[B1_PROJECT_NAME_MAX]=0;
}
static void b1_save_named(void){
 if(!b1_storage_name_valid(edit_name)){project_error=-11;return;}
 if(b1_saved.state!=B1_STORAGE_READY && b1_saved.state!=B1_STORAGE_EMPTY){project_error=-12;return;}
 /* V3.6: stop audio BEFORE copying the project. The audio callback mutates
  * engine and performance state; capturing it during playback could tear a
  * project across two different DMA blocks. The existing flash operation
  * already stops audio, so this adds no extra stop/start cycle. */
 b1_stop_for_write();
 b1_pending_save=b1_saved.archive;
 b1_project_capture(&b1_pending_save.projects[project_cursor],&b1_keys,&b1_audio);
 for(unsigned i=0;i<=B1_PROJECT_NAME_MAX;i++)b1_pending_save.names[project_cursor][i]=edit_name[i];
 b1_pending_save.mask|=(1u<<project_cursor);
 int rc=b1_storage_commit(&b1_saved,&b1_flash_io,&b1_pending_save);
 b1_resume_after_write();
 if(rc){project_error=(int8_t)(rc<-100?-100:rc);return;}
 project_error=0;dirty_project=0u;ui_page=4u;
 project_notice=1u;project_notice_until=b1_ms+1600u;
}
static uint8_t bounded_add(uint8_t n,int32_t delta,uint8_t low,uint8_t high){
 int32_t v=(int32_t)n+delta;if(v<low)v=low;if(v>high)v=high;return (uint8_t)v;
}
static int8_t bounded_add_signed(int8_t n,int32_t d,int8_t low,int8_t high){
 int32_t v=(int32_t)n+d;if(v<low)v=low;if(v>high)v=high;return (int8_t)v;
}
static void b1_refresh_leds(void){
 /* FM1 hardware ids 0..13 are function LEDs, NOT note LEDs.
  * Applying SCALE GUIDE to them made FX/SCL/ENV/etc unexpectedly flash.
  * The note range is 14..40 and the page light has its own fixed policy. */
 b1_apply_leds(&b1_keys,fm1_in.notes,ui_page,fm1_led_key);
}
static void b1_new_project(void){b1_release_all(&b1_keys,&b1_audio);b1_panic(&b1_audio);b1_reset(&b1_audio);b1_performance_init(&b1_keys);chord_cursor=20u;chord_quality=b1_keys.chord_map[chord_cursor];dirty_project=1u;}
/* Project transitions can zero tens of kilobytes and are intentionally not
 * allowed inside b1_controls' IRQ-masked edits. The audio IRQ keeps servicing
 * DMA with silence until the transition commits; other interrupts keep running.
 * No flash access, DMA engine reset, or 330ms codec re-prime is needed here. */
static void b1_execute_project_transition(void){
 uint8_t op=b1_project_transition_requested;
 if(!op)return;
 b1_project_transition_requested=0u;
 fm1_irq_off();
 b1_audio_active=0u;
 /* Drop MIDI events belonging to the previous project. New incoming
  * events during the transition are dropped again at commit, preventing
  * obsolete note-ons from firing immediately after the restart. */
 mi_r=mi_w;midi_in_overflow=0u;
 fm1_irq_enable_all();
 if(op==1u){
  if(b1_saved.archive.mask&(1u<<b1_transition_slot)){
   if(b1_project_apply(&b1_saved.archive.projects[b1_transition_slot],&b1_keys,&b1_audio)){
    dirty_project=0u;project_error=0;project_notice=2u;project_notice_until=b1_ms+1600u;
   }else project_error=-13;
  }else project_error=-13;
 }else if(op==2u){
  b1_new_project();project_new_armed=0u;project_error=0;
 }
 /* Restart musical output with the existing anti-pop ramp. Restarts on the
  * next ALNK half event; the DMA buffer itself was never reinitialized. */
 fade=0u;
 fm1_irq_off();
 mi_r=mi_w;midi_in_overflow=0u;
 b1_audio_active=1u;
 fm1_irq_enable_all();
 b1_ui_dirty=1u;
}
static void b1_controls(uint32_t pressed){
 /* Stop ordinary editor, synth and save shortcuts on the destructive
  * consent screen. REC or HOME is always a non-destructive NO. */
 if(ui_page==8u){
  if(pressed&((1u<<13)|(1u<<8))){reclaim_armed=0u;ui_page=4u;b1_ui_dirty=1u;}
  else if(pressed&(1u<<9)){
   if(!reclaim_armed){reclaim_armed=1u;b1_ui_dirty=1u;}
   else {reclaim_armed=0u;save_requested=2u;}
  }
  return;
 }
 /* Recognize HOME+FX regardless of the order of the two presses.
  * A short grace window keeps HOME from silencing an in-progress chord
  * while the performer is trying to open ENGINE MODE. */
 uint8_t combo_toggle=0u;
 pressed=b1_mode_keys_filter(&b1_mode_key_state,pressed,fm1_in.buttons,b1_ms,&combo_toggle);
 if(combo_toggle){
  ui_page=(ui_page==9u?0u:9u);
  b1_ui_dirty=1u;
  return;
 }
 uint8_t changed=0;
 /* Always preserve the two recovery boot shortcuts and the HOME escape. */
 if(pressed&(1u<<8)){
  /* HOME is also an emergency note-off: no lingering stuck tones when the
   * player releases the keyboard and returns to the drawbar page. */
  b1_release_all(&b1_keys,&b1_audio);b1_panic(&b1_audio);
  ui_page=0;b1_audio_diag=0u;b1_test_tone=0u;
  project_new_armed=0;changed=1;
 }
 if(pressed&(1u<<2)){ui_page=1;changed=1;}
 if(pressed&(1u<<4)){ui_page=2;changed=1;}
 if(pressed&(1u<<5)){if(ui_page!=6u){b1_release_all(&b1_keys,&b1_audio);chord_quality=b1_keys.chord_map[chord_cursor];project_error=0;}ui_page=6;changed=1;}
 if(pressed&(1u<<6)){ui_page=3;changed=1;}
 if(pressed&(1u<<11)){ui_page=4;changed=1;}
 if(pressed&(1u<<7)){ui_page=5;b1_audio_diag=0u;b1_test_tone=0u;changed=1;}
 if(pressed&3u){
  int8_t direction=(int8_t)((!!(pressed&2u))-(!!(pressed&1u)));
  if(b1_shift_octave(&b1_keys,&b1_audio,direction,fm1_in.notes)){changed=1;dirty_project=1u;}
 }
 if(pressed&(1u<<3)){
  if(ui_page==3){struct b1_range *r=&b1_keys.range[selected_range];r->type=(uint8_t)(r->type==B1_ORGAN?B1_CHORD:r->type==B1_CHORD?B1_DISABLED:B1_ORGAN);
   if(r->type==B1_CHORD)b1_keys.chord_mode=2u;
   changed=1;dirty_project=1;}
  /* SEL is physical front-panel SEL/SCL button (id 3), not ALGORITHM. */
  if(ui_page==4 && (b1_saved.archive.mask&(1u<<project_cursor))){
   /* Defer big state copy/reset until IRQs are restored. */
   b1_transition_slot=project_cursor;
   b1_project_transition_requested=1u;
   changed=1;
  }
 }
 if(ui_page==7u){
  if(pressed&(1u<<9)){save_requested=1u;changed=1;}
  /* REC cancels the name editor, never overwriting a saved project. */
  if(pressed&(1u<<13)){ui_page=4u;changed=1;}
 }else if(ui_page==4u){
  if(pressed&(1u<<9)){b1_name_edit_begin();changed=1;}
  if(pressed&(1u<<13)){
   if(project_new_armed){b1_project_transition_requested=2u;project_new_armed=0;}
   else project_new_armed=1;
   changed=1;
  }
 }else if(pressed&(1u<<9)){ui_page=4u;changed=1;}
 if(ui_page!=4u)project_new_armed=0;
 int32_t enc[4]={fm1_enc_take(2),fm1_enc_take(3),fm1_enc_take(4),fm1_enc_take(5)};
 int32_t sel=fm1_enc_take(0),alg=fm1_enc_take(1);(void)fm1_enc_take(6);
 if(ui_page==9u){
  if(enc[0]){
   uint8_t next=(uint8_t)(enc[0]>0?B1_OSC_SAW:B1_OSC_ORGAN);
   if(next!=b1_audio.osc_mode){b1_audio.osc_mode=next;dirty_project=1u;changed=1u;}
  }
  if(pressed&(1u<<3)){
   b1_audio.osc_mode^=1u;dirty_project=1u;changed=1u;
  }
 }else if(ui_page==0){
  if(sel){uint8_t next=bounded_add(drawbar_page,sel,0,2);if(next!=drawbar_page){drawbar_page=next;changed=1;}}
  if(alg){uint8_t next=bounded_add(b1_keys.selected_bank,alg,0,2);if(next!=b1_keys.selected_bank){b1_keys.selected_bank=next;changed=1;dirty_project=1u;}}
  for(uint8_t k=0;k<4;k++){
   uint8_t i=(uint8_t)(drawbar_page*4u+k);
   if(i<9u&&enc[k]){uint8_t before=b1_audio.drawbars[b1_keys.selected_bank][i];
    uint8_t after=bounded_add(before,enc[k],0,8);
    b1_drawbar(&b1_audio,b1_keys.selected_bank,i,after);
    changed|=(uint8_t)(before!=after);dirty_project=1;}
  }
 }else if(ui_page==1){
  if(sel){fx_page=bounded_add(fx_page,sel,0,5);changed=1;}
  if(fx_page==0u){
  if(enc[0]){b1_audio.rotor_fast=enc[0]>0;changed=1;dirty_project=1;}
  if(enc[1]){b1_audio.drive=bounded_add(b1_audio.drive,enc[1],0,100);changed=1;dirty_project=1;}
  if(enc[2]){b1_audio.click=bounded_add(b1_audio.click,enc[2],0,100);changed=1;dirty_project=1;}
  if(enc[3]){b1_audio.leakage=bounded_add(b1_audio.leakage,enc[3],0,100);changed=1;dirty_project=1;}
  }else if(fx_page==1u){
   if(enc[0]){b1_audio.cv_mode=bounded_add(b1_audio.cv_mode,enc[0],0,6);changed=1;dirty_project=1;}
   if(enc[1]){b1_audio.rotor_width=bounded_add(b1_audio.rotor_width,enc[1],0,100);changed=1;dirty_project=1;}
   if(enc[2]){b1_audio.rotor_mix=bounded_add(b1_audio.rotor_mix,enc[2],0,100);changed=1;dirty_project=1;}
   if(enc[3]){b1_audio.cv_depth=bounded_add(b1_audio.cv_depth,enc[3],0,100);changed=1;dirty_project=1;}
  }else if(fx_page==2u){
   if(enc[0]){b1_audio.sound_mode=bounded_add(b1_audio.sound_mode,enc[0],0,1);changed=1;dirty_project=1;}
   if(enc[1]){b1_audio.amp_gain=bounded_add(b1_audio.amp_gain,enc[1],0,100);changed=1;dirty_project=1;}
   if(enc[2]){b1_audio.amp_tone=bounded_add(b1_audio.amp_tone,enc[2],0,100);changed=1;dirty_project=1;}
   if(enc[3]){b1_audio.cabinet_mix=bounded_add(b1_audio.cabinet_mix,enc[3],0,100);changed=1;dirty_project=1;}
  }else if(fx_page==3u){
   uint8_t *speeds[3]={&b1_audio.rotor_slow_pct,&b1_audio.rotor_fast_pct,&b1_audio.rotor_drum_pct};
   for(uint8_t k=0;k<3u;k++)if(enc[k]){
    uint8_t old=*speeds[k]?*speeds[k]:100u;
    *speeds[k]=bounded_add(old,enc[k],50,150);changed=1;dirty_project=1;
   }
   if(enc[3]){b1_audio.rotor_mix=bounded_add(b1_audio.rotor_mix,enc[3],0,100);changed=1;dirty_project=1;}
  }else if(fx_page==4u){
   if(enc[0]){b1_audio.delay_mix=bounded_add(b1_audio.delay_mix,enc[0],0,100);changed=1;dirty_project=1;}
   if(enc[1]){b1_audio.delay_time=bounded_add(b1_audio.delay_time,enc[1],0,100);changed=1;dirty_project=1;}
   if(enc[2]){b1_audio.delay_feedback=bounded_add(b1_audio.delay_feedback,enc[2],0,90);changed=1;dirty_project=1;}
   if(enc[3]){b1_audio.delay_spread=bounded_add(b1_audio.delay_spread,enc[3],0,100);changed=1;dirty_project=1;}
  }else{
   if(enc[0]){b1_audio.reverb_type=bounded_add(b1_audio.reverb_type,enc[0],0,4);changed=1;dirty_project=1;}
   if(enc[1]){b1_audio.reverb_mix=bounded_add(b1_audio.reverb_mix,enc[1],0,100);changed=1;dirty_project=1;}
   if(enc[2]){b1_audio.reverb_decay=bounded_add(b1_audio.reverb_decay,enc[2],0,100);changed=1;dirty_project=1;}
   if(enc[3]){b1_audio.reverb_damp=bounded_add(b1_audio.reverb_damp,enc[3],0,100);changed=1;dirty_project=1;}
  }
 }else if(ui_page==2){
  if(enc[0]){b1_audio.percussion_enabled=enc[0]>0;changed=1;dirty_project=1;}
  if(enc[1]){b1_audio.percussion_harmonic=enc[1]>0?3:2;changed=1;dirty_project=1;}
 }else if(ui_page==3){
  struct b1_range *r=&b1_keys.range[selected_range];
  if(enc[0]){selected_range=bounded_add(selected_range,enc[0],0,B1_RANGES-1);r=&b1_keys.range[selected_range];changed=1;}
  if(enc[1]){r->lo=bounded_add(r->lo,enc[1],0,26);if(r->hi<r->lo)r->hi=r->lo;changed=1;dirty_project=1;}
  if(enc[2]){r->hi=bounded_add(r->hi,enc[2],r->lo,26);changed=1;dirty_project=1;}
  if(enc[3]){
   if(r->type==B1_CHORD)r->attack=bounded_add(r->attack,enc[3],0,100);
   else r->octave=bounded_add_signed(r->octave,enc[3],-4,4);
   changed=1;dirty_project=1;
  }
  if(alg){r->bank=bounded_add(r->bank,alg,0,B1_BANKS-1);changed=1;dirty_project=1;}
 }else if(ui_page==4){
  if(enc[0]||sel){project_cursor=bounded_add(project_cursor,enc[0]+sel,0,B1_PROJECT_SLOTS-1);changed=1;}
 }else if(ui_page==7){
  if(enc[0]){project_cursor=bounded_add(project_cursor,enc[0],0,B1_PROJECT_SLOTS-1);b1_name_edit_begin();changed=1;}
  if(enc[1]||sel){name_cursor=bounded_add(name_cursor,enc[1]+sel,0,B1_PROJECT_NAME_MAX-1u);changed=1;}
  if(enc[2]){b1_name_change(enc[2]);changed=1;}
  if(enc[3]){edit_name[name_cursor]=' ';changed=1;}
 }else if(ui_page==6){
  if(enc[0]){chord_cursor=bounded_add(chord_cursor,enc[0],0,B1_KEYS-1);chord_quality=b1_keys.chord_map[chord_cursor];changed=1;}
  if(enc[1]){chord_quality=bounded_add(chord_quality,enc[1],0,B1_CHORD_TYPES-1);changed=1;}
  if(enc[2]){
   if(!b1_set_chord_enabled(&b1_keys,&b1_audio,(uint8_t)(enc[2]>0))){project_error=-15;}
   else {project_error=0;dirty_project=1u;selected_range=2u;
    for(uint8_t ci=0u;ci<B1_RANGES;ci++)if(b1_keys.range[ci].type==B1_CHORD){selected_range=ci;break;}
    chord_cursor=b1_keys.range[selected_range].lo;chord_quality=b1_keys.chord_map[chord_cursor];}
   changed=1;
  }
  if(enc[3]&&b1_chord_enabled(&b1_keys)){
   uint8_t next=bounded_add(b1_chord_attack(&b1_keys),enc[3],0,100);
   if(b1_set_chord_attack(&b1_keys,next)){dirty_project=1u;changed=1;}
  }
 }else if(ui_page==5){
  if(sel){b1_audio_diag=sel>0u;changed=1; if(!b1_audio_diag)b1_test_tone=0u;}
  if(b1_audio_diag){
   if(enc[0]){b1_test_tone=enc[0]>0; b1_release_all(&b1_keys,&b1_audio);b1_panic(&b1_audio);changed=1;}
   if(enc[1]){b1_force_safe=enc[1]>0;changed=1;}
  }else{
   if(enc[0]){b1_keys.scale_root=bounded_add(b1_keys.scale_root,enc[0],0,11);changed=1;dirty_project=1;}
   if(enc[1]){b1_keys.scale_type=bounded_add(b1_keys.scale_type,enc[1],0,B1_SCALE_COUNT-1u);changed=1;dirty_project=1;}
   if(enc[2]){b1_keys.keylock=enc[2]>0;changed=1;dirty_project=1;}
   if(enc[3]){b1_keys.guide=enc[3]>0;changed=1;dirty_project=1;}
  }
 }
 if(changed)b1_ui_dirty=1;
}
void fm1_cstart(void){
 fm1_audio_output_enable(0u);
 fm1_time_init();fm1_reset_reason();fm1_wdt_arm(0x0du);fm1_irq_init();
 uint32_t *d,*s;
 for(d=_bss_start;d<_bss_end;d++)*d=0;
 for(d=_pool_start;d<_pool_end;d++)*d=0;
 for(s=_data_load,d=_data_start;d<_data_end;s++,d++)*d=*s;
 for(s=_rt_load,d=_rt_start;d<_rt_end;s++,d++)*d=*s;
 fm1_mailbox_clear();fm1_guard_enable(FM1_GUARD_STACK|FM1_GUARD_WRITE|FM1_GUARD_BUS|FM1_GUARD_PC);
 lcd_init();fm1_input_init();fm1_timer5_start(isr_timer5,4);fm1_irq_enable_all();fm1_delay_ms(50);
 /* OCT- bypasses the app and enters the existing bootloader: stock/other firmware restore path. */
 if(fm1_in.buttons&1u){fm1_audio_output_enable(0u);fm1_irq_off();fm1_enter_uboot();}
 safe_mode=(uint8_t)((fm1_in.buttons&(1u<<13))!=0u); /* REC at power -> silent recovery UI */
 b1_performance_init(&b1_keys);b1_reset(&b1_audio);chord_cursor=20u;chord_quality=b1_keys.chord_map[chord_cursor];
 /* USB comes up BEFORE any project-sector scan. REC boot is a genuinely
  * minimal update/recovery mode and never touches project flash at all. */
 usb_start();
 if(!safe_mode){
  /* Never erase flash at boot. A fully occupied private journal region
   * can be reclaimed ONLY with explicit two-step on-device confirmation. */
  (void)b1_storage_open(&b1_saved,&b1_flash_io);
  if(b1_saved.state==B1_STORAGE_LOCKED && b1_saved.active_bank<0)ui_page=8u;
 }
 if(safe_mode)b1_screen(); /* Direct safe UI, never animate over recovery. */
 else {b1_startup_animation();b1_screen();}
 if(usb.uboot_req){fm1_audio_output_enable(0u);usb_detach();fm1_irq_off();fm1_enter_uboot();}
 if(!safe_mode && !usb.ota_req){
  fm1_adc_init();
  int32_t start_gain=fm1_adc_read(FM1_ADC_MASTER);
  if(start_gain>=0)b1_master_target=(uint16_t)((start_gain*32767)/1023);
  b1_master_smoothed=0;
  /* Boot in known-bounded sound mode. Promote full B3/Leslie only after
   * clean DMA timing is observed and the keyboard is idle. If full DSP
   * misses once, SAFE DSP remains for this boot instead of muting audio. */
  b1_audio.cpu_relief=1u;b1_safe_latched=0u;b1_good_dma_blocks=0u;
  for(uint32_t i=0;i<HALF_FRAMES*4;i++)b1_dma[i]=0;
  fm1_irq_off();fm1_audio_init(b1_dma,HALF_FRAMES*2,isr_alnk0,3);b1_audio_active=1;fm1_irq_enable_all();
  /* Silent DMA priming before the analog output gate opens. */
  uint32_t ready=b1_ms+330u;while((int32_t)(b1_ms-ready)<0){fm1_wdt_feed();usb_retry(b1_ms);}
  fm1_audio_output_enable(1u);output_ready=1;
 }
 uint32_t ui_checked=0,last_lcd_frame=0,last_diag_frame=0;
 uint32_t last_background_ms=UINT32_MAX;
 for(;;){
  /* V4.2: CPU0 used to busy-spin between every TIMER5/UI tick. All note
   * scanning, USB IRQ servicing and ALNK DMA live in enabled interrupts.
   * One main-loop housekeeping iteration per millisecond is sufficient;
   * UBOOT, OTA and realtime underrun flags bypass the throttle immediately.
   * Interrupts MUST be on whenever the pi32v2 IDLE instruction executes. */
  uint32_t now_ms=b1_ms;
  if(b1_can_idle_main(now_ms,last_background_ms,usb.uboot_req,usb.ota_req,b1_overrun)){
   b1_cpu_wait_for_irq();
   continue;
  }
  last_background_ms=now_ms;
  fm1_wdt_feed();usb_retry(now_ms);
  if(usb.uboot_req){fm1_audio_output_enable(0u);usb_detach();fm1_irq_off();fm1_enter_uboot();}
  /* OTA identity requests are serviced outside audio to avoid blocking DMA. */
  ota_service();
  if(usb.ota_req){
   usb.ota_req=0u;
   fm1_audio_output_enable(0u);
   fm1_irq_off();b1_audio_active=0u;fm1_audio_stop();fm1_irq_enable_all();
   b1_updating=1u;b1_ota_step=1u;b1_ota_error=0;
   b1_screen();
   int update_status=ota_session();
   b1_updating=0u;
   /* A successful update does not return. A failure must stay silent,
    * preserve projects, and keep the USB identity/OTA retry available. */
   b1_ota_step=9u;b1_ota_error=update_status;
   safe_mode=1u;b1_screen();
   continue;
  }
  /* A single dropped audio half MUST NOT permanently mute the instrument.
   * Raise diagnostic status and request reduced DSP load; leave DMA running.
   * A real driver fault remains visible instead of silently appearing as no sound. */
  if(b1_overrun && b1_audio_active){b1_overrun=0u;b1_cpu_relief=1u;b1_ui_dirty=1u;}
  /* No gradual full/fast toggling while a key is held. A full DSP trial
   * is allowed only after >=2s of consistently fast blocks and silence. */
  if(b1_audio_active && !b1_safe_latched){
   if(b1_force_safe){b1_audio.cpu_relief=1u;}
   else if(b1_good_dma_blocks>=690u && !b1_keys.pressed){
    uint8_t any=0u;
    for(uint8_t v=0;v<B1_VOICES;v++)any|=b1_audio.voice[v].active;
    /* Never auto-promote to high-cost Leslie/cabinet DSP on the FM1.
     * Earlier releases repeatedly missed DMA deadlines, replayed stale
     * PCM and sounded like never-ending ringing. Keep the stable path
     * until a separately measured full-DSP mode is ready. */
    (void)any;
   }
  }
  if(b1_ms-ui_checked<10u){b1_cpu_wait_for_irq();continue;}
  ui_checked=b1_ms;
  if(!safe_mode && (b1_ms%50u)<10u){
   int32_t pot=fm1_adc_read(FM1_ADC_MASTER);
   if(pot>=0)b1_master_target=(uint16_t)((pot*32767)/1023);
  }
  if((b1_ms%40u)<10u)b1_refresh_leds();
  uint32_t released=0u;
  uint32_t pressed=fm1_input_edges(&released);
  (void)released;
  /* Input HAL buffers short taps across UI periods: OCT +/- no longer misses
   * a tap whose press and release both happen between 10ms UI polls. */
  if(ui_page==6u){
   uint32_t chord_edges=fm1_input_note_edges();
   if(chord_edges){
    for(uint8_t k=0;k<B1_KEYS;k++)if(chord_edges&(1u<<k)){
     /* Only active selectors may be assigned; K3 ON creates a selector range for new projects. */
     if(!b1_chord_enabled(&b1_keys)){
      project_error=0; /* With ASSIGN OFF, keep the normal keyboard playable. */
     }else if(b1_assign_chord_key(&b1_keys,k,chord_quality)){
      chord_cursor=k;dirty_project=1u;project_error=0;
     }else project_error=-14;
     b1_ui_dirty=1u;
    }
   }
  }else (void)fm1_input_note_edges();
  if(safe_mode){if(pressed&(1u<<8))b1_screen();continue;}
  /* Normal control edits can be realtime-atomic, but flash save requires
   * interrupts temporarily ON between individual guarded erase/program calls.
   * b1_save_named() stops audio before touching the flash. */
  uint32_t edit_begin=fm1_ticks();
  fm1_irq_off();b1_controls(pressed);
  uint32_t edit_ticks=(uint32_t)(fm1_ticks()-edit_begin);
  fm1_irq_enable_all();
  if(edit_ticks>b1_ui_irq_peak_ticks)b1_ui_irq_peak_ticks=edit_ticks;
  b1_execute_project_transition();
  if(save_requested){
   uint8_t operation=save_requested;save_requested=0u;
   if(operation==2u){
    b1_stop_for_write();
    int rc=b1_storage_reclaim_confirmed(&b1_saved,&b1_flash_io,1u);
    b1_resume_after_write();
    project_error=(int8_t)rc;
    ui_page=4u;
   }else b1_save_named();
   b1_ui_dirty=1u;
  }
  /* Bound LCD transfers to 20Hz, coalescing rapid encoder events.
   * Audio DMA and USB take priority over visual feedback. The pending flag
   * is not cleared until the frame has actually been submitted. */
  if(project_notice && (int32_t)(project_notice_until-b1_ms)<=0){
   project_notice=0u;b1_ui_dirty=1u;
  }
  if(b1_audio_diag && ui_page==5u && (uint32_t)(b1_ms-last_diag_frame)>=500u){
   last_diag_frame=b1_ms;b1_ui_dirty=1u;
  }
  if(b1_ui_dirty && (uint32_t)(b1_ms-last_lcd_frame)>=50u){
   b1_ui_dirty=0;last_lcd_frame=b1_ms;b1_screen();
  }
 }
}
