#include "streamoo_spectrum.h"
#include "streamoo_pcm.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
int main(void) {
    struct guarded { uint64_t before;streamoo_spectrum_t s;uint64_t after; } *g=calloc(1,sizeof *g);
    assert(g);g->before=0x12345678;g->after=0xabcdef;
    int32_t lr[4096];
    for(unsigned n=1024;n<=2048;n*=2){
        assert(streamoo_spectrum_init(&g->s,n,48000));
        for(unsigned i=0;i<n;++i){
            int32_t v=(int32_t)lround(1073741824.0*sin(2*3.141592653589793*20*i/n));
            lr[2*i]=v;lr[2*i+1]=-v;
        }
        assert(streamoo_spectrum_analyze(&g->s,lr,n));
        assert(fabs(g->s.amplitude[20]-.5)<.00001);
        assert(!streamoo_spectrum_analyze(&g->s,lr,n+1));
        for(unsigned b=0;b<24;++b)assert(g->s.bands[b]<=32767);
        assert(g->before==0x12345678&&g->after==0xabcdef);
    }
    /* Real PCM/gain/tap, identical pre-gain spectrum at all three volumes. */
    streamoo_pcm_block_t storage[32],tap_storage[2];
    int32_t raw[480],out[480],block[480];float reference[513];
    const uint8_t volumes[]={0,50,100};
    for(unsigned v=0;v<3;++v){
        streamoo_pcm_ring_t source,tap;streamoo_pcm_pipeline_t pipe;
        assert(streamoo_pcm_ring_init(&source,storage,32));
        assert(streamoo_pcm_ring_init(&tap,tap_storage,2));
        assert(streamoo_pcm_pipeline_init(&pipe,&source,&tap));
        for(unsigned i=0;i<240;++i){block[2*i]=(i%2?10000:-10000)*65536;block[2*i+1]=-block[2*i];}
        for(unsigned b=0;b<24;++b)assert(streamoo_pcm_push(&source,block,b));
        uint32_t gain;assert(streamoo_audio_gain_volume(volumes[v],&gain));
        for(unsigned b=0;b<5;++b){
            assert(streamoo_pcm_fill(&pipe,out,240,gain));
            assert(streamoo_pcm_pop(&tap,raw,NULL));
            unsigned frames=b==4?64:240;
            memcpy(lr+b*480,raw,frames*2*sizeof *raw);
            if(v==0)for(unsigned i=0;i<480;++i)assert(out[i]==0);
        }
        assert(streamoo_spectrum_init(&g->s,1024,48000));
        assert(streamoo_spectrum_analyze(&g->s,lr,1024));
        if(v==0)memcpy(reference,g->s.amplitude,sizeof reference);
        else assert(!memcmp(reference,g->s.amplitude,sizeof reference));
    }
    free(g);return 0;
}
