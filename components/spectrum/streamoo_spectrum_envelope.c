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
    }
    s->last_us=now;
}
