#include "streamoo_ballistics.h"
void streamoo_ballistics_init(streamoo_ballistics_t *b, int64_t now) {
    if (b) *b=(streamoo_ballistics_t){.last_us=now};
}
void streamoo_ballistics_feed(streamoo_ballistics_t *b, const streamoo_levels_result_t *r) {
    if (!b || !r || !r->valid) return;
    for(unsigned ch=0;ch<2;++ch) {
        b->target_rms[ch]=r->rms[ch];
        if(r->peak[ch]>b->pending_peak[ch]) b->pending_peak[ch]=r->peak[ch];
    }
}
static uint32_t approach(uint32_t from, uint32_t to, uint32_t dt, uint32_t tau) {
    uint64_t delta=from>to ? from-to : to-from;
    uint32_t step=(uint32_t)(delta*dt/(tau+dt));
    return from>to ? from-step : from+step;
}
bool streamoo_ballistics_update(streamoo_ballistics_t *b, int64_t now,
                                uint16_t rms[2], uint16_t peak[2]) {
    if(!b || !rms || !peak || now<b->last_us || now-b->last_us<100000) return false;
    uint32_t dt=(uint32_t)((now-b->last_us)/1000);
    if(dt>1000) dt=1000;
    for(unsigned ch=0;ch<2;++ch) {
        uint32_t target=(uint32_t)b->target_rms[ch]*65536u;
        b->rms_q16[ch]=approach(b->rms_q16[ch],target,dt,
                               target>b->rms_q16[ch]?30:150);
        uint32_t p=(uint32_t)b->pending_peak[ch]*65536u;
        if(p>=b->peak_q16[ch]) {
            b->peak_q16[ch]=p;
            b->hold_until_us[ch]=now+150000;
        } else if(now>b->hold_until_us[ch]) {
            int64_t start=b->last_us>b->hold_until_us[ch]?b->last_us:b->hold_until_us[ch];
            uint32_t release=(uint32_t)((now-start)/1000);
            if(release>1000)release=1000;
            b->peak_q16[ch]=approach(b->peak_q16[ch],p,release,300);
        }
        if(b->peak_q16[ch]<b->rms_q16[ch])b->peak_q16[ch]=b->rms_q16[ch];
        rms[ch]=(uint16_t)(b->rms_q16[ch]/65536u);
        peak[ch]=(uint16_t)(b->peak_q16[ch]/65536u);
        b->pending_peak[ch]=0;
    }
    b->last_us=now;
    return true;
}
