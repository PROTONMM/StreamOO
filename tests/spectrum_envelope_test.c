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
 streamoo_spectrum_envelope_feed(NULL,target,600000);
 streamoo_spectrum_envelope_feed(&s,NULL,600000);
 return 0;
}
