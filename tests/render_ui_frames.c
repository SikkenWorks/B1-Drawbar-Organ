/* Render exact native B1 FM1 UI frames from the SAME shared b1_ui_render
 * function that is linked into the target build. No mockup or HTML. */
#include "../firmware/b1_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint16_t fb[240u*240u];
static struct b1_engine e;
static struct b1_performance p;
static void emit(const char *dir,const char *name,struct b1_ui_state *s){
 char filename[500];
 snprintf(filename,sizeof(filename),"%s/%s.rgb565",dir,name);
 b1_ui_render(fb,s);
 FILE *f=fopen(filename,"wb");if(!f){perror(filename);exit(1);}
 if(fwrite(fb,sizeof(fb),1,f)!=1){perror("fwrite");exit(1);}
 fclose(f);
 printf("%s\n",filename);
}
int main(int argc,char **argv){
 if(argc!=2){fprintf(stderr,"usage: render_ui_frames OUTPUT_DIR\n");return 2;}
 b1_reset(&e);b1_performance_init(&p);
 struct b1_ui_state s={0};s.audio=&e;s.keys=&p;
 emit(argv[1],"01-drawbars-upper-page1",&s);
 p.global_octave=5;emit(argv[1],"01b-octave-plus1",&s);p.global_octave=4;
 s.drawbar_page=1;emit(argv[1],"02-drawbars-upper-page2",&s);
 s.drawbar_page=2;emit(argv[1],"03-drawbars-upper-page3",&s);
 s.drawbar_page=0;p.selected_bank=1;
 emit(argv[1],"04-drawbars-lower",&s);
 p.selected_bank=0;s.page=1;
 emit(argv[1],"05-leslie-drive",&s);
 e.click=50u;emit(argv[1],"05b-key-click-50",&s);
 e.click=100u;emit(argv[1],"05c-key-click-100",&s);
 e.click=0u;
 s.fx_page=1;emit(argv[1],"06-chorus-vibrato-off",&s);
 e.cv_mode=5;emit(argv[1],"07-chorus-vibrato-c2",&s);
 e.cv_mode=0;s.page=1;s.fx_page=2;e.sound_mode=B1_SOUND_ROCK;
 emit(argv[1],"07b-rock-lord-amp",&s);
 e.sound_mode=B1_SOUND_B3;emit(argv[1],"07c-b3-leslie-amp",&s);
 s.page=1;s.fx_page=3;e.rotor_slow_pct=85;e.rotor_fast_pct=118;e.rotor_drum_pct=96;
 emit(argv[1],"07d-rotor-speed-finetune",&s);
 s.fx_page=4;e.delay_mix=52;e.delay_time=44;e.delay_feedback=31;e.delay_spread=60;
 emit(argv[1],"07e-delay",&s);
 s.fx_page=5;e.reverb_type=1;e.reverb_mix=48;e.reverb_decay=43;e.reverb_damp=57;
 emit(argv[1],"07f-reverb-spring",&s);
 e.reverb_type=2;emit(argv[1],"07g-reverb-room",&s);
 e.reverb_type=3;emit(argv[1],"07h-reverb-hall",&s);
 e.reverb_type=4;emit(argv[1],"07i-reverb-cathedral",&s);
 e.reverb_type=0;e.reverb_mix=0;e.delay_mix=0;
 e.rotor_slow_pct=100;e.rotor_fast_pct=100;e.rotor_drum_pct=100;
 s.fx_page=0;s.page=0;p.selected_bank=2;emit(argv[1],"04b-drawbars-solo",&s);
 s.fx_page=0;s.page=2;emit(argv[1],"08-percussion",&s);
 s.page=3;emit(argv[1],"09-ranges-organ",&s);
 s.selected_range=2;emit(argv[1],"10-ranges-chord",&s);
 p.range[2].attack=72;emit(argv[1],"10b-ranges-chord-humanized",&s);
 s.page=6;s.chord_cursor=21;s.chord_quality=p.chord_map[21];emit(argv[1],"11-chord-assignment",&s);
 p.chord_map[21]=B1_MIN7;s.chord_quality=B1_MIN7;
 emit(argv[1],"11b-chord-assigned-min7",&s);
 s.page=5;s.project_error=0;emit(argv[1],"12-scale-keylock",&s);
 p.scale_root=0;p.scale_type=0;emit(argv[1],"12a-scale-c-chromatic",&s);
 p.scale_root=0;p.scale_type=1;emit(argv[1],"12b-scale-c-major",&s);
 p.scale_root=3;p.scale_type=2;emit(argv[1],"12c-scale-dsharp-minor",&s);
 p.scale_root=6;p.scale_type=3;emit(argv[1],"12d-scale-fsharp-lydian",&s);
 p.guide=0;p.scale_root=0;p.scale_type=4;emit(argv[1],"12e-scale-c-dorian-guide-off",&s);
 p.scale_root=7;p.scale_type=6;emit(argv[1],"12f-scale-g-mixolydian",&s);
 p.scale_root=9;p.scale_type=7;emit(argv[1],"12g-scale-a-locrian",&s);
 p.scale_root=0;p.scale_type=21;emit(argv[1],"12h-scale-c-minor-pentatonic",&s);
 p.scale_root=2;p.scale_type=26;emit(argv[1],"12i-scale-d-whole-half-dim",&s);
 p.scale_type=45;p.scale_root=6;emit(argv[1],"12j-scale-fsharp-blues-oct",&s);
 s.audio_diag=1;s.audio_test=1;s.audio_safe=1;
 s.audio_blocks=1530;s.audio_misses=0;
 emit(argv[1],"12k-audio-diagnostic",&s);
 s.audio_diag=0;
 s.page=4;s.project_storage_state=B1_STORAGE_EMPTY;
 emit(argv[1],"13-project-menu",&s);
 static char names[B1_PROJECT_SLOTS][B1_PROJECT_NAME_MAX+1u];
 s.project_storage_state=B1_STORAGE_READY;s.project_names=(const char (*)[B1_PROJECT_NAME_MAX+1u])names;
 s.project_mask=(1u<<0)|(1u<<11);
 strcpy(names[0],"DEEP PURPLE");strcpy(names[11],"BLACK NIGHT");
 emit(argv[1],"13b-project-saved-01",&s);
 s.project_notice=1;emit(argv[1],"13h-project-save-feedback",&s);
 s.project_notice=2;emit(argv[1],"13i-project-load-feedback",&s);s.project_notice=0;
 s.project_cursor=11;emit(argv[1],"13c-project-saved-12",&s);
 s.page=7;s.edit_name=names[0];s.project_cursor=0;s.name_cursor=4;
 emit(argv[1],"13d-project-name-editor",&s);
 s.page=4;s.project_storage_state=B1_STORAGE_LOCKED;
 emit(argv[1],"13e-storage-locked",&s);
 s.safe_mode=0;s.ota_step=0;s.page=8u;s.reclaim_armed=0u;
 emit(argv[1],"13f-storage-erase-confirm-no",&s);
 s.reclaim_armed=1u;emit(argv[1],"13g-storage-erase-confirm-yes",&s);
 s.reclaim_armed=0u;s.page=4u;
 s.safe_mode=1;emit(argv[1],"14-safe-mode",&s);
 s.safe_mode=0;s.ota_step=3;s.ota_error=0;emit(argv[1],"15-ota-check-stage",&s);
 s.ota_step=9;s.ota_error=-5;emit(argv[1],"16-ota-check-error",&s);
 return 0;
}
