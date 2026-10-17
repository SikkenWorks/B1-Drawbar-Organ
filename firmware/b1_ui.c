/* SPDX-License-Identifier: GPL-3.0-only
 * SikkenWorks B1 V1.1. Native 240x240 RGB565 UI.
 * The FM1 app and all pixel-exact screenshot tests call this same renderer.
 * Text uses an on-device, embedded two-weight proportional Inter bitmap.
 * Full frame is assembled offscreen and handed to the LCD only when ready.
 */
#include "b1_ui.h"
#include "b1_font.h"
#include <stdint.h>

#define BG 0x0841u
#define PANEL 0x10A2u
#define TRACK 0x2965u
#define LINE 0x4208u
#define MUTED 0x94B2u
#define WHITE 0xEF7Du
#define AMBER 0xFD40u
#define PINK 0xEA78u
static uint16_t *target;
static uint16_t swap16(uint16_t c){return (uint16_t)((c<<8)|(c>>8));}
static void rect(int x,int y,int w,int h,uint16_t col){
 if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}
 if(x>=240||y>=240||w<=0||h<=0)return;
 if(x+w>240)w=240-x;
 if(y+h>240)h=240-y;
 const uint16_t v=swap16(col);
 for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)target[j*240+i]=v;
}
static void set_pixel(int x,int y,uint16_t c,uint8_t a){
 if((unsigned)x>=240u||(unsigned)y>=240u||!a)return;
 uint16_t *p=&target[y*240+x];
 if(a==3){*p=swap16(c);return;}
 uint16_t old=swap16(*p);
 unsigned r=((old>>11)&31u)*(3u-a)+((c>>11)&31u)*a;
 unsigned g=((old>>5)&63u)*(3u-a)+((c>>5)&63u)*a;
 unsigned b=(old&31u)*(3u-a)+(c&31u)*a;
 *p=swap16((uint16_t)(((r/3u)<<11)|((g/3u)<<5)|(b/3u)));
}
static uint8_t find_char(char ch){
 if(ch>='a'&&ch<='z')ch=(char)(ch-32);
 for(uint8_t i=0;B1_FONT_CHARACTERS[i];i++)if(ch==B1_FONT_CHARACTERS[i])return i;
 return 0u;
}
static void text(const char *s,int x,int y,int headline,uint16_t c){
 while(*s&&x<239){
  uint8_t i=find_char(*s++);
  unsigned w,h,yo,adv,off;
  const uint8_t *pix;
  if(headline){
   const b1_font_headline_glyph_t *g=&b1_font_headline_glyphs[i];
   w=g->w;h=g->h;yo=g->y;adv=g->adv;off=g->off;pix=b1_font_headline_pixels;
  }else{
   const b1_font_small_glyph_t *g=&b1_font_small_glyphs[i];
   w=g->w;h=g->h;yo=g->y;adv=g->adv;off=g->off;pix=b1_font_small_pixels;
  }
  for(unsigned j=0;j<h;j++)for(unsigned k=0;k<w;k++){
   unsigned n=j*w+k;
   uint8_t a=(uint8_t)((pix[off+n/4u]>>(6u-2u*(n%4u)))&3u);
   if(a)set_pixel(x+(int)k,y+(int)yo+(int)j,c,a);
  }
  x+=(int)adv;
 }
}
static int text_width(const char *s,int headline){
 int width=0;
 while(*s){uint8_t i=find_char(*s++);width+=headline?b1_font_headline_glyphs[i].adv:b1_font_small_glyphs[i].adv;}
 return width;
}
static void right_text(const char *s,int right,int y,int headline,uint16_t col){text(s,right-text_width(s,headline),y,headline,col);}
static void number(int val,char *buf){
 char rev[12];unsigned v;int n=0,j=0;
 if(val<0){buf[n++]='-';v=(unsigned)(-(val+1))+1u;}else v=(unsigned)val;
 do{rev[j++]=(char)('0'+v%10u);v/=10u;}while(v&&j<11);
 while(j)buf[n++]=rev[--j];
 buf[n]=0;
}
static void val_number(int val,int right,int y,int headline,uint16_t col){char s[16];number(val,s);right_text(s,right,y,headline,col);}
static void top(const char *name,const char *section){
 text("SIKKENWORKS",12,7,0,MUTED);
 right_text("B1",228,3,1,WHITE);
 rect(12,31,216,1,LINE);
 text(name,12,42,1,WHITE);
 if(section){right_text(section,228,69,0,MUTED);}
}
static void foot(const char *left,const char *right){
 rect(12,221,216,1,LINE);
 text(left,12,225,0,MUTED);
 if(right)right_text(right,228,225,0,AMBER);
}
static const char *banks[3]={"UPPER","LOWER","SOLO"};
static const char *qualities[14]={"MAJ","MIN","MAJ7","MIN7","DOM7","DIM","DIM7","AUG","SUS2","SUS4","ADD9","MINADD9","MIN7B5","MAJ9"};
/* These names correspond exactly to the four pitch-class masks in
 * b1_performance.c. Values in saved projects remain unchanged. */
static const char *const b1_roots[12]={
 "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};
static const char *const b1_scales[B1_SCALE_COUNT]={
 "CHROMATIC",
 "MAJOR",
 "MINOR",
 "LYDIAN",
 "DORIAN",
 "PHRYGIAN",
 "MIXOLYDIAN",
 "LOCRIAN",
 "HARM MINOR",
 "MELOD MIN",
 "HARM MAJOR",
 "DBL HARM",
 "PHRYG DOM",
 "LYD DOM",
 "ALTERED",
 "LOCRIAN #2",
 "DORIAN B2",
 "LYDIAN AUG",
 "MIXO B6",
 "MAJOR LOC",
 "MAJ PENTA",
 "MIN PENTA",
 "MAJ BLUES",
 "MIN BLUES",
 "WHOLE TONE",
 "DIM H-W",
 "DIM W-H",
 "AUGMENTED",
 "BEBOP DOM",
 "BEBOP MAJ",
 "BEBOP DOR",
 "BEBOP MIN",
 "HUNG MIN",
 "NEAP MIN",
 "NEAP MAJ",
 "PERSIAN",
 "ROMANIAN MIN",
 "ENIGMATIC",
 "HIRAJOSHI",
 "IN SEN",
 "IWATO",
 "YO",
 "EGYPTIAN",
 "PELOG",
 "PROMETHEUS",
 "BLUES OCT",
 "SPANISH 8",
 "LYDIAN #2"
};
const char *b1_root_name(uint8_t root){return b1_roots[root%12u];}
const char *b1_scale_name(uint8_t scale){return b1_scales[scale%B1_SCALE_COUNT];}
static const char *feet[9]={"16'","5 1/3'","8'","4'","2 2/3'","2'","1 3/5'","1 1/3'","1'"};
static void field(const char *name,const char *v,int y,uint16_t colour){
 text(name,14,y+2,0,MUTED);
 right_text(v,224,y-3,1,colour);
 rect(14,y+24,212,1,LINE);
}
static void field_num(const char *name,int v,int y,uint16_t colour){char buf[16];number(v,buf);field(name,buf,y,colour);}

void b1_ui_render(uint16_t *frame,const struct b1_ui_state *s){
 target=frame;rect(0,0,240,240,BG);
 if(s->ota_step){
  top("FIRMWARE UPDATE",0);
  text("USB OTA IN PROGRESS",14,83,0,AMBER);
  field_num("STAGE",s->ota_step,106,WHITE);
  field_num("RESULT",(int)s->ota_error,146,s->ota_error?PINK:WHITE);
  text("DO NOT DISCONNECT",14,192,0,PINK);
  foot("USB ACTIVE",0);
 }else if(s->safe_mode){
  top("SAFE MODE",0);
  text("USB MIDI ACTIVE",14,87,0,AMBER);
  text("PROJECT STORAGE LOCKED",14,116,0,WHITE);
  text("OCT- AT BOOT: UBOOT",14,149,0,MUTED);
  foot("FELUCCA HAL",0);
 }else if(s->page==8u){
  top("STORAGE CONFLICT",0);
  text("UNKNOWN FLASH DATA",14,87,0,PINK);
  text("BACK UP PROJECTS FIRST",14,108,0,WHITE);
  text("ERASE TWO B1 SLOTS?",14,132,0,AMBER);
  text("ONLY B1 JOURNAL AREA",14,155,0,MUTED);
  text(s->reclaim_armed?"SAVE AGAIN: CONFIRM":"SAVE: YES (TWICE)",14,183,0,AMBER);
  text("REC / HOME: NO",14,202,0,WHITE);
  foot("OTHER DATA MAY BE LOST",0);
 }else if(s->page==0u){
  uint8_t bank=s->keys->selected_bank;
  top(banks[bank],"DRAWBARS");
  char octave[8];number((int)s->keys->global_octave-4,octave);
  text("OCT",14,69,0,MUTED);text(octave,44,69,0,AMBER);
  /* Mini registration: 0 at top (pushed in), 8 at bottom (pulled out). */
  for(uint8_t h=0;h<9u;h++){
   const int x=15+(int)h*24;
   uint8_t v=s->audio->drawbars[bank][h];
   int active=(h/4u==s->drawbar_page);
   rect(x,82,16,25,TRACK);rect(x+2,84,12,21,PANEL);
   if(v)rect(x+3,84,10,(int)v*2,active?AMBER:MUTED);
   rect(x,109,16,2,active?AMBER:LINE);
  }
  for(uint8_t k=0;k<4u;k++){
   uint8_t pos=(uint8_t)(s->drawbar_page*4u+k);if(pos>=9u)break;
   int cx=(s->drawbar_page==2u)?118:25+(int)k*53;
   uint8_t v=s->audio->drawbars[bank][pos];
   text(feet[pos],cx-10,117,0,WHITE);
   /* Hammond-style vertical visualization: greater draw = farther down. */
   rect(cx+4,143,9,51,TRACK);
   for(uint8_t tick=0;tick<9u;tick++)rect(cx+1,144+(int)tick*6,15,1,LINE);
   rect(cx-7,142+(int)v*6,31,8,AMBER);
   char buf[16];number(k+1,buf);text("K",cx-13,199,0,MUTED);text(buf,cx-4,199,0,MUTED);
   val_number(v,cx+33,195,1,WHITE);
  }
  char n[14];number(s->drawbar_page+1,n);
  char page[9]="PAGE 1/3";page[5]=n[0];
  foot("SELECT: PAGE",page);
 }else if(s->page==1u){
  if(s->fx_page==0u){
   top("LESLIE / DRIVE","FX 1/6");
   field("K1  ROTOR",s->audio->rotor_fast?"FAST":"SLOW",86,AMBER);
   field_num("K2  DRIVE",s->audio->drive,120,WHITE);
   field_num("K3  KEY CLICK",s->audio->click,154,WHITE);
   field_num("K4  LEAKAGE",s->audio->leakage,188,WHITE);
  }else if(s->fx_page==1u){
   static const char *modes[7]={"OFF","V1","V2","V3","C1","C2","C3"};
   top("CHORUS / VIB","FX 2/6");
   field("K1  MODE",modes[s->audio->cv_mode%7u],86,AMBER);
   field_num("K2  ROTOR SIZE",s->audio->rotor_width,120,WHITE);
   field_num("K3  ROTOR MIX",s->audio->rotor_mix,154,WHITE);
   field_num("K4  CV DEPTH",s->audio->cv_depth,188,WHITE);
  }else if(s->fx_page==2u){
   top("ORGAN / AMP","FX 3/6");
   field("K1  MODE",s->audio->sound_mode==B1_SOUND_ROCK?"ROCK / LORD":"B3 / LESLIE",86,AMBER);
   field_num("K2  AMP GAIN",s->audio->amp_gain,120,WHITE);
   field_num("K3  AMP TONE",s->audio->amp_tone,154,WHITE);
   field_num("K4  CABINET",s->audio->cabinet_mix,188,WHITE);
  }else if(s->fx_page==3u){
   top("ROTOR SPEED","FX 4/6");
   field_num("K1  SLOW %",s->audio->rotor_slow_pct?s->audio->rotor_slow_pct:100u,86,AMBER);
   field_num("K2  FAST %",s->audio->rotor_fast_pct?s->audio->rotor_fast_pct:100u,120,WHITE);
   field_num("K3  DRUM %",s->audio->rotor_drum_pct?s->audio->rotor_drum_pct:100u,154,WHITE);
   field_num("K4  ROTOR MIX",s->audio->rotor_mix,188,WHITE);
  }else if(s->fx_page==4u){
   top("DELAY","FX 5/6");
   field_num("K1  MIX",s->audio->delay_mix,86,AMBER);
   field_num("K2  TIME",s->audio->delay_time,120,WHITE);
   field_num("K3  FEEDBACK",s->audio->delay_feedback,154,WHITE);
   field_num("K4  STEREO",s->audio->delay_spread,188,WHITE);
  }else{
   static const char *types[]={"OFF","SPRING","ROOM","HALL","CATHEDRAL"};
   top("REVERB","FX 6/6");
   field("K1  TYPE",types[s->audio->reverb_type%5u],86,AMBER);
   field_num("K2  MIX",s->audio->reverb_mix,120,WHITE);
   field_num("K3  DECAY",s->audio->reverb_decay,154,WHITE);
   field_num("K4  DAMP",s->audio->reverb_damp,188,WHITE);
  }
  foot("SELECT: FX PAGE",0);
 }else if(s->page==2u){
  top("PERCUSSION",0);
  field("K1  PERCUSSION",s->audio->percussion_enabled?"ON":"OFF",89,AMBER);
  field_num("K2  HARMONIC",s->audio->percussion_harmonic,132,WHITE);
  foot("2ND / 3RD HARMONIC",0);
 }else if(s->page==3u){
  const struct b1_range *r=&s->keys->range[s->selected_range];
  top("KEY RANGES",0);
  field_num("K1  GROUP",s->selected_range+1,86,AMBER);
  field_num("K2  START",r->lo,120,WHITE);
  field_num("K3  END",r->hi,154,WHITE);
  if(r->type==B1_CHORD)field_num("K4  ATTACK",r->attack,188,AMBER);
  else field_num("K4  OCTAVE",r->octave,188,WHITE);
  /* SELECT changes existing range type; no invented range mode. */
  if(r->type==B1_ORGAN)foot("ORGAN RANGE",banks[r->bank]);
  else if(r->type==B1_CHORD)foot("CHORD RANGE",0);
  else foot("RANGE DISABLED",0);
 }else if(s->page==4u){
  top("PROJECT MENU","12 SLOTS");
  field_num("K1  SLOT",s->project_cursor+1,82,AMBER);
  if(s->project_mask&(1u<<s->project_cursor)){
   text("PROJECT NAME",14,121,0,MUTED);
   text(s->project_names[s->project_cursor],14,141,1,WHITE);
  }else text("EMPTY SLOT",14,135,1,MUTED);
  if(s->project_storage_state==B1_STORAGE_LOCKED)
    text("STORAGE LOCKED",14,174,0,PINK);
  else if(s->project_storage_state==B1_STORAGE_ERROR)
    text("FLASH READ ERROR",14,174,0,PINK);
  else if(s->project_error)
    field_num("SAVE ERROR",(int)s->project_error,165,PINK);
  else if(s->project_notice==1u)text("PROJECT SAVED",14,174,1,AMBER);
  else if(s->project_notice==2u)text("PROJECT LOADED",14,174,1,AMBER);
  else text(s->dirty_project?"UNSAVED CHANGES":"READY",14,177,0,AMBER);
  text(s->project_new_armed?"REC AGAIN: NEW":"REC: NEW PROJECT",14,194,0,s->project_new_armed?PINK:MUTED);
  foot("SAVE: NAME / WRITE","SEL: LOAD");
 }else if(s->page==7u){
  top("SAVE PROJECT","NAME EDIT");
  field_num("K1  SLOT",s->project_cursor+1,80,AMBER);
  text("PROJECT NAME",14,116,0,MUTED);
  text(s->edit_name,14,136,0,WHITE);
  /* Caret is based on the SAME font advance as the actual name text. */
  char prefix[B1_PROJECT_NAME_MAX+1u];
  for(uint8_t i=0;i<s->name_cursor;i++)prefix[i]=s->edit_name[i]?s->edit_name[i]:' ';
  prefix[s->name_cursor]=0;
  int caret=14+text_width(prefix,0);
  rect(caret,153,7,2,PINK);
  text("K2 POS   K3 CHAR",14,170,0,MUTED);
  text("K4 ERASE",14,185,0,MUTED);
  if(s->project_error)field_num("WRITE ERROR",s->project_error,196,PINK);
  else text("SAVE: COMMIT  REC: BACK",14,202,0,AMBER);
  foot("TWO-BANK FLASH JOURNAL",0);
 }else if(s->page==6u){
  top("CHORD ASSIGN",0);
  field_num("ASSIGNED KEY",s->chord_cursor+1,85,WHITE);
  text("K2  CHORD TYPE",14,129,0,MUTED);
  text(qualities[s->chord_quality%B1_CHORD_TYPES],15,151,1,AMBER);
  if(s->project_error==-14)text("KEY OUTSIDE CHORD RANGE",14,187,0,PINK);
  else text("PRESS A CHORD-RANGE KEY",14,187,0,AMBER);
  foot("K1: BROWSE  KEY: ASSIGN",0);
 }else if(s->page==5u){
  if(s->audio_diag){
   top("AUDIO DIAG",0);
   field("K1  TEST TONE",s->audio_test?"ON":"OFF",83,AMBER);
   field("K2  SAFE DSP",s->audio_safe?"ON":"AUTO",118,WHITE);
   field_num("DMA BLOCKS",(int)(s->audio_blocks%10000u),153,WHITE);
   field_num("DMA MISSES",(int)(s->audio_misses%10000u),188,s->audio_misses?PINK:WHITE);
   foot("SELECT: SCALE",0);
  }else{
  top("SCALE / KEYLOCK",0);
  field("K1  ROOT",b1_root_name(s->keys->scale_root),86,WHITE);
  field("K2  SCALE",b1_scale_name(s->keys->scale_type),120,WHITE);
  field("K3  KEYLOCK",s->keys->keylock?"ON":"OFF",154,AMBER);
  field("K4  GUIDE",s->keys->guide?"ON":"OFF",188,AMBER);
  foot("SELECT: AUDIO DIAG",0);
  }
 }else{
  top("B1 / SYSTEM",0);
 }
 if(s->overrun)rect(12,34,216,3,PINK);
}
