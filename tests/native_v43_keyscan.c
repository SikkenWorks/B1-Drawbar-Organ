/* V4.3 host differential test: same raw samples through baseline and fast
 * input/key debounce logic. Tests exact notes, buttons, counter state,
 * press edges and release timing. Does not emulate 74HC595 electrical timing. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "b1_keyscan_fast.h"
#define COLS 11u
#define KEYS 41u
static const int8_t map[4][11] = {
 {5,11,4,10,3,9,2,8,-1,-1,-1},
 {34,35,36,37,38,40,39,13,7,6,12},
 {23,24,25,26,27,28,29,30,31,32,33},
 {0,1,15,14,17,16,19,18,20,21,22}
};
struct machine {
 uint8_t raw[COLS],cnt[KEYS],pending[COLS];
 uint32_t notes,buttons,pressed,released,notes_pressed,note_t0[27];
 uint32_t visits;
};
static void key(struct machine *s,unsigned id,unsigned closed,unsigned tick){
 uint8_t *c=&s->cnt[id];
 unsigned note=id>=14u;
 uint32_t bit=1u<<(note ? id-14u:id);
 unsigned on=!!((note?s->notes:s->buttons)&bit);
 ++s->visits;
 if(closed==on){*c=0;return;}
 if(!on && note && *c==0u)s->note_t0[id-14u]=tick;
 if(++*c < (on?8u:2u))return;
 *c=0;
 if(note){if(!on)s->notes_pressed|=bit;s->notes^=bit;}
 else {s->buttons^=bit;if(!on)s->pressed|=bit;else s->released|=bit;}
}
static void scan(struct machine *s,unsigned col,uint8_t sample,unsigned tick,int fast){
 uint8_t old=s->raw[col],pending=0;
 s->raw[col]=sample;
 if(fast && !b1_keyscan_needs_processing(sample,old,s->pending[col],b1_key_row_mask[col]))return;
 for(unsigned row=1;row<5;row++){
   int id=map[row-1][col];
   if(id>=0){key(s,(unsigned)id,(sample>>row)&1u,tick);pending|=(uint8_t)!!s->cnt[id];}
 }
 s->pending[col]=pending;
}
static uint32_t rng=0x8ff16d49u;
static uint32_t rnd(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(void){
 struct machine a={0},b={0};uint8_t columns[COLS]={0};
 uint32_t changes=0;
 for(unsigned tick=0;tick<1100000u;tick++){
   unsigned col=tick%COLS;
   /* Long stable periods and randomized overlapping, simultaneous contact
    * changes including rapid bounce (< threshold) and fast key retriggers. */
   if(tick%750000u==0u){for(unsigned c=0;c<COLS;c++)columns[c]=0;}
   uint32_t r=rnd();
   if((r&8191u)<15u || ((tick/11000u)%19u==0u && (r&63u)<5u)){
    columns[col]^=(uint8_t)(1u << (1u+(r>>12)%4u));++changes;
   }
   if((r&2047u)==0u)columns[col]^=1u; /* unrelated encoder row */
   scan(&a,col,columns[col],tick,0);scan(&b,col,columns[col],tick,1);
   assert(a.notes==b.notes && a.buttons==b.buttons);
   assert(a.pressed==b.pressed && a.released==b.released && a.notes_pressed==b.notes_pressed);
   assert(memcmp(a.cnt,b.cnt,KEYS)==0);
   assert(memcmp(a.note_t0,b.note_t0,sizeof a.note_t0)==0);
   assert(memcmp(a.raw,b.raw,sizeof a.raw)==0);
   assert(memcmp(a.pending,b.pending,sizeof a.pending)==0);
   /* Simulate UI consumption identically. */
   if(tick%100u==0u){a.pressed=b.pressed=0;a.released=b.released=0;a.notes_pressed=b.notes_pressed=0;}
 }
 for(unsigned c=0;c<COLS;c++){
  uint8_t mask=0u;
  for(unsigned row=1;row<5;row++)if(map[row-1][c]>=0)mask|=(uint8_t)(1u<<row);
  assert(mask==b1_key_row_mask[c]);
  uint32_t expected=0xffffu^(1u<<c)^(c<2u?(1u<<(11u+c)):0u);
  assert(expected==b1_column_shift_word[c]);
 }
 assert(b.visits<a.visits);
 printf("PASS V4.3 exact 1,100,000-column differential scans; %u input changes; baseline %u key visits, fast %u (%.1f%% fewer)\n",changes,a.visits,b.visits,100.0*(1.0-(double)b.visits/(double)a.visits));
 return 0;
}
