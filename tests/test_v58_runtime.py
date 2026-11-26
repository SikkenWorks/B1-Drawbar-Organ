"""Exercise current runtime code with host peripherals and run software regressions."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
src=(root/'firmware/b1_main.c').read_text()
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)
    def run(name,files,flags=()):
        exe=path/name
        subprocess.run(['cc','-std=c11','-O2','-Ifirmware','-Ifirmware/hal',*flags,*files,'-o',exe],check=True,cwd=root)
        subprocess.run([exe],check=True,cwd=root)
    run('dac',['tests/native_v58_output.c'])
    dsp=['firmware/b1_engine.c','firmware/b1_performance.c']
    run('storage',['tests/native_v49_autosave.c','firmware/b1_storage.c',*dsp],['-DB1_STORAGE_DYNAMIC=1'])
    run('scan',['tests/native_v43_keyscan.c'])
    run('startup',['tests/native_startup_test.c','firmware/b1_startup.c','firmware/b1_ui.c',*dsp])
    run('chord',['tests/native_v28_live_attack.c',*dsp])
    run('rock',['tests/native_v38_rock_percussion.c',*dsp])
    run('fx',['tests/native_v23_max_fx_stress.c',*dsp])
    # Compile the actual copy loop, replacing only hardware reads and the ISA barrier.
    copy=src[src.index('static uint8_t b1_copy_to_dma'):src.index('static void b1_note_dma_miss')]
    copy=copy.replace('__asm__ volatile("csync" ::: "memory");','')
    mock=path/'copy.c'
    mock.write_text('''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define HALF_FRAMES 256u
static unsigned checks,loss;
static int fm1_audio_half_owned(uint32_t half){assert(half==1);return ++checks<loss;}
'''+copy+'''
int main(void){
 int32_t in[512],out[512];
 for(loss=1;loss<=18;loss++){
  checks=0;
  for(unsigned i=0;i<512;i++){in[i]=i;out[i]=-1;}
  assert(b1_copy_to_dma(1,out,in)==(loss>17));
  for(unsigned i=0;i<512;i++)assert(out[i]==(i/32+1<loss?(int)i:-1));
 }
 puts("PASS: ownership loss before every DMA group and at the final check");
}
''')
    run('copy',[mock])
    # Exercise the autosave decision and new-project entry points, substituting
    # only hardware/flash operations. Queue a note exactly at the final lock.
    service=src[src.index('static uint8_t b1_autosave_quiet'):src.index('static const char b1_name_chars')]
    new=src[src.index('static void b1_new_project(void){'):src.index('/* Project transitions can zero')]
    mock=path/'service.c'
    mock.write_text('''
#include <assert.h>
#include <stdio.h>
#include "b1_storage.h"
#define B1_AUTO_DEBOUNCE_MS 5000u
#define B1_AUTO_MIN_INTERVAL_MS 60000u
static struct b1_performance b1_keys;
static struct b1_engine b1_audio;
static struct b1_storage b1_saved;
static struct b1_project_archive b1_pending_save;
static struct b1_storage_io b1_flash_io;
static struct {int ota_req,uboot_req;} usb;
static struct {uint32_t notes;} fm1_in;
static unsigned b1_ms=60000,b1_auto_last_edit_ms,b1_auto_last_attempt_ms,project_notice_until;
static int safe_mode,b1_updating,project_io_busy,save_requested,b1_project_transition_requested,
 b1_audio_active=1,mi_r,mi_w,midi_in_overflow,b1_test_tone,b1_auto_pending,b1_auto_attempted,
 project_notice,b1_ui_dirty,project_error,chord_cursor,chord_quality,dirty_project;
static unsigned stops,resumes,writes,inject,locked;
static void fm1_irq_off(void){locked=1;if(inject)mi_w++;}
static void fm1_irq_enable_all(void){locked=0;}
static void b1_stop_for_write(void){assert(locked);stops++;locked=0;}
static void b1_resume_after_write(void){resumes++;}
int b1_storage_commit(struct b1_storage *s,const struct b1_storage_io *io,
 const struct b1_project_archive *a){(void)io;writes++;s->archive=*a;return 0;}
'''+service+new+'''
int main(void){
 b1_audio.cpu_relief=1;b1_new_project();assert(b1_audio.cpu_relief==1);
 b1_audio.cpu_relief=0;b1_new_project();assert(b1_audio.cpu_relief==0);
 b1_saved.state=B1_STORAGE_READY;b1_saved.archive.live_valid=1;
 b1_project_capture(&b1_saved.archive.live,&b1_keys,&b1_audio);
 b1_auto_pending=1;b1_autosave_service();
 assert(!b1_auto_pending && !stops && !writes && !resumes && !b1_auto_attempted);
 b1_auto_pending=1;b1_audio.drive=83;b1_test_tone=1;b1_autosave_service();
 assert(!stops && b1_auto_pending);
 b1_test_tone=0;inject=1;b1_autosave_service();
 assert(!locked && !stops && b1_auto_pending);
 inject=0;mi_r=mi_w;b1_autosave_service();
 assert(!locked && stops==1 && writes==1 && resumes==1 && !b1_auto_pending);
 assert(b1_saved.archive.live.drive==83);
 puts("PASS: unchanged autosave, diagnostic tone, MIDI arriving at save boundary, real edit and project DSP reset");
}
''')
    run('service',[mock,*dsp])
