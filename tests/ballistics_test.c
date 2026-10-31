#include "streamoo_ballistics.h"
#include <assert.h>
int main(void){
 streamoo_ballistics_t b;uint16_t rms[2]={0},peak[2]={0};
 streamoo_ballistics_init(&b,0);
 streamoo_levels_result_t r={.valid=true,.rms={20000,10000},.peak={30000,15000}};
 streamoo_ballistics_feed(&b,&r);
 assert(!streamoo_ballistics_update(&b,99999,rms,peak));
 assert(streamoo_ballistics_update(&b,100000,rms,peak));
 assert(rms[0]>=15000&&rms[0]<16000&&rms[1]>=7500&&rms[1]<8000&&peak[0]==30000&&peak[1]==15000);
 uint16_t before=rms[0];
 assert(!streamoo_ballistics_update(&b,150000,rms,peak)&&rms[0]==before);
 assert(!streamoo_ballistics_update(&b,0,rms,peak));
 for(int i=2;i<=4;++i){
  streamoo_ballistics_feed(&b,&r);assert(streamoo_ballistics_update(&b,i*100000,rms,peak));
  assert(rms[0]>before&&rms[0]<20000);before=rms[0];
 }
 r=(streamoo_levels_result_t){.valid=true};
 for(int i=5;i<=8;++i){
  streamoo_ballistics_feed(&b,&r);assert(streamoo_ballistics_update(&b,i*100000,rms,peak));
  assert(rms[0]<before&&rms[0]>0);
  if(i==5)assert(peak[0]==30000);
  if(i>=6)assert(peak[0]<30000);
  if(i==6)assert(rms[0]<8000); /* most of the fall visible within200ms */
  before=rms[0];
 }
 streamoo_ballistics_feed(&b,&r);assert(streamoo_ballistics_update(&b,900000,rms,peak));
 assert(peak[0]<30000&&peak[0]>rms[0]);
 /* Peaks from windows BETWEEN display ticks must survive. */
 streamoo_ballistics_init(&b,0);
 r=(streamoo_levels_result_t){.valid=true,.peak={30000,10000}};streamoo_ballistics_feed(&b,&r);
 r.peak[0]=5000;streamoo_ballistics_feed(&b,&r);
 assert(streamoo_ballistics_update(&b,100000,rms,peak)&&peak[0]==30000);
 r.valid=false;r.peak[0]=32767;streamoo_ballistics_feed(&b,&r);
 assert(streamoo_ballistics_update(&b,200000,rms,peak)&&peak[0]==30000);
 /* New session starts empty, no previous hold. */
 streamoo_ballistics_init(&b,200000);
 assert(streamoo_ballistics_update(&b,300000,rms,peak)&&!rms[0]&&!peak[0]);
 streamoo_ballistics_feed(NULL,&r);streamoo_ballistics_feed(&b,NULL);
 assert(!streamoo_ballistics_update(NULL,400000,rms,peak));
 return 0;
}
