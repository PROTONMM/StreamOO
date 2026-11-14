#include "streamoo_spectrum_envelope.h"
void streamoo_spectrum_envelope_init(streamoo_spectrum_envelope_t *s,int64_t now) {
    if(s)*s=(streamoo_spectrum_envelope_t){.last_us=now};
}
void streamoo_spectrum_envelope_feed(streamoo_spectrum_envelope_t *s,
                                     const uint16_t target[STREAMOO_SPECTRUM_BANDS],
                                     int64_t now) {
    if(!s||!target||now<s->last_us)return;
    int64_t dt=now-s->last_us;
    if(dt>320000)dt=320000;
    uint32_t drop=(uint32_t)(((uint64_t)dt*32767u+319999u)/320000u);
    for(unsigned b=0;b<STREAMOO_SPECTRUM_BANDS;++b) {
        uint16_t decayed=s->bands[b]>drop ? (uint16_t)(s->bands[b]-drop):0;
        uint16_t value=target[b]>32767 ? 32767:target[b];
        s->bands[b]=value>decayed ? value:decayed;
        if(s->bands[b]>=s->peaks[b] && s->bands[b]) {
            s->peaks[b]=s->bands[b];
            s->hold_until_us[b]=now>INT64_MAX-500000 ? INT64_MAX:now+500000;
        } else if(now>s->hold_until_us[b]) {
            int64_t from=s->last_us>s->hold_until_us[b] ? s->last_us:s->hold_until_us[b];
            int64_t elapsed=now-from;
            if(elapsed>2000000)elapsed=2000000;
            uint32_t fall=(uint32_t)(((uint64_t)elapsed*32767u+1999999u)/2000000u);
            uint16_t peak=s->peaks[b]>fall ? s->peaks[b]-fall:0;
            s->peaks[b]=peak>s->bands[b] ? peak:s->bands[b];
        }
    }
    s->last_us=now;
}
