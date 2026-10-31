#include "streamoo_spectrum_stream.h"
#include <math.h>
#include <string.h>
bool streamoo_spectrum_stream_init(streamoo_spectrum_stream_t *s) {
    if(!s)return false;
    memset(s,0,sizeof *s);
    return streamoo_spectrum_init(&s->fft,STREAMOO_SPECTRUM_LIVE_SIZE,48000);
}
void streamoo_spectrum_stream_reset(streamoo_spectrum_stream_t *s) {
    if(s){s->filled=0;s->seen=false;}
}
bool streamoo_spectrum_stream_feed(streamoo_spectrum_stream_t *s,const int32_t *lr,
                                   size_t frames,uint32_t sequence,bool calculate) {
    if(!s||!lr||!frames||frames>240||s->fft.size!=STREAMOO_SPECTRUM_LIVE_SIZE)return false;
    if(s->seen && sequence!=s->previous_sequence+UINT32_C(1)){
        s->filled=0;++s->gaps;
    }
    s->seen=true;s->previous_sequence=sequence;
    bool produced=false;
    for(size_t i=0;i<frames;++i){
        s->lr[2*s->filled]=lr[2*i];s->lr[2*s->filled+1]=lr[2*i+1];
        if(++s->filled==STREAMOO_SPECTRUM_LIVE_SIZE){
            if(calculate){
                produced=streamoo_spectrum_analyze(&s->fft,s->lr,STREAMOO_SPECTRUM_LIVE_SIZE);
                if(produced)++s->windows;
            }else ++s->skipped;
            memmove(s->lr,s->lr+STREAMOO_SPECTRUM_LIVE_SIZE,
                    STREAMOO_SPECTRUM_LIVE_SIZE*sizeof *s->lr);
            s->filled=STREAMOO_SPECTRUM_LIVE_SIZE/2;
        }
    }
    return produced;
}
uint16_t streamoo_spectrum_display_level(uint16_t value){
    if(value<=32)return 0;
    if(value>=32767)return 32767;
    float db=20.0f*log10f((float)value/32767.0f);
    float level=(db+60.0f)/60.0f;
    if(level<=0)return 0;
    /* Keep the -60..0 dBFS calibration, but suppress the low-level carpet
     * so genuinely dominant bands are readable on the small OLED. */
    return (uint16_t)lroundf(level*sqrtf(level)*32767.0f);
}
