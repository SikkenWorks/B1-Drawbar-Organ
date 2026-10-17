/* Same b1_startup_render linked into the FM1 binary. */
#include "../firmware/b1_startup.h"
#include <stdio.h>
#include <stdlib.h>
static struct b1_engine e;
static struct b1_performance p;
static uint16_t pixels[240u*240u];
int main(int argc,char **argv){
 if(argc<3){fprintf(stderr,"usage: render_startup_frames OUT_DIR MS ...\n");return 2;}
 b1_reset(&e);b1_performance_init(&p);
 struct b1_ui_state home={0};home.audio=&e;home.keys=&p;
 for(int i=2;i<argc;i++){
  unsigned long ms=strtoul(argv[i],0,10);
  char name[512];snprintf(name,sizeof(name),"%s/boot-%04lu.rgb565",argv[1],ms);
  b1_startup_render(pixels,&home,ms);
  FILE *f=fopen(name,"wb");if(!f){perror(name);return 3;}
  if(fwrite(pixels,sizeof(pixels),1,f)!=1){perror(name);fclose(f);return 4;}
  fclose(f);
 }
 return 0;
}
