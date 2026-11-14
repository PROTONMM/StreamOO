#include "streamoo_spectrum_envelope.h"
#include <assert.h>
#include <string.h>
int main(void){
 streamoo_spectrum_envelope_t s;uint16_t target[24]={0};
 streamoo_spectrum_envelope_init(&s,0);
 target[4]=32767;target[5]=16000;
 streamoo_spectrum_envelope_feed(&s,target,100000);
 assert(s.bands[4]==32767&&s.bands[5]==16000); /* Immediate rise, no averaging. */
 memset(target,0,sizeof target);
 streamoo_spectrum_envelope_feed(&s,target,200000);
 assert(s.bands[4]>22000&&s.bands[4]<23000&&s.bands[5]>5000);
 streamoo_spectrum_envelope_feed(&s,target,300000);
 assert(s.bands[4]>12000&&!s.bands[5]);
 streamoo_spectrum_envelope_feed(&s,target,420000);
 assert(!s.bands[4]); /* Complete fall within320ms of the peak. */
 target[4]=24000;streamoo_spectrum_envelope_feed(&s,target,500000);
 assert(s.bands[4]==24000);
 streamoo_spectrum_envelope_init(&s,500000);assert(!s.bands[4]);
 streamoo_spectrum_envelope_feed(&s,target,400000);assert(!s.bands[4]);
 streamoo_spectrum_envelope_feed(&s,target,500000);assert(s.bands[4]==24000);
 /* Independent500ms hold, then2s full-range fall; zero input keeps valid FFTs. */
 streamoo_spectrum_envelope_init(&s,0);memset(target,0,sizeof target);
 target[0]=32767;streamoo_spectrum_envelope_feed(&s,target,0);
 assert(s.peaks[0]==32767&&s.hold_until_us[0]==500000);
 memset(target,0,sizeof target);
 streamoo_spectrum_envelope_feed(&s,target,500000);
 assert(!s.bands[0]&&s.peaks[0]==32767);
 streamoo_spectrum_envelope_feed(&s,target,1500000);
 assert(s.peaks[0]>=16382&&s.peaks[0]<=16384);
 target[0]=30000;streamoo_spectrum_envelope_feed(&s,target,1600000);
 assert(s.peaks[0]==30000&&s.hold_until_us[0]==2100000);
 memset(target,0,sizeof target);
 streamoo_spectrum_envelope_feed(&s,target,2100000);
 assert(s.peaks[0]==30000&&!s.bands[0]);
 streamoo_spectrum_envelope_t before=s;
 streamoo_spectrum_envelope_feed(&s,target,2000000);assert(!memcmp(&before,&s,sizeof s));
 streamoo_spectrum_envelope_feed(&s,target,4100000);assert(!s.peaks[0]);
 target[1]=32767;streamoo_spectrum_envelope_feed(&s,target,4200000);
 assert(!s.peaks[0]&&s.peaks[1]==32767); /* Independent bands. */
 streamoo_spectrum_envelope_init(&s,4300000);
 assert(!s.peaks[1]&&!s.bands[1]&&!s.hold_until_us[1]);
 streamoo_spectrum_envelope_feed(NULL,target,600000);
 streamoo_spectrum_envelope_feed(&s,NULL,600000);
 return 0;
}
