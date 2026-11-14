#include "streamoo_levels.h"
#include "streamoo_pcm.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static int32_t samples[480];
static void constant(int32_t l, int32_t r) {
    for (unsigned i=0;i<240;++i) { samples[2*i]=l; samples[2*i+1]=r; }
}
int main(int argc, char **argv) {
    assert(argc==2);
    int test=atoi(argv[1]); streamoo_levels_t s;
    assert(streamoo_levels_init(&s,1920));
    if(test==0) {
        streamoo_levels_t before=s;
        assert(!streamoo_levels_init(NULL,1920));
        assert(!streamoo_levels_init(&s,0));
        assert(!streamoo_levels_init(&s,48001));
        assert(!streamoo_levels_process(NULL,samples,240,0));
        assert(!streamoo_levels_process(&s,NULL,240,0));
        assert(!streamoo_levels_process(&s,samples,0,0));
        assert(!streamoo_levels_process(&s,samples,241,0));
        assert(!memcmp(&s,&before,sizeof s));
        s.window_frames=0; before=s;
        assert(!streamoo_levels_process(&s,samples,240,0));
        assert(!memcmp(&s,&before,sizeof s));
    } else if(test==1) {
        for(unsigned seq=0;seq<8;++seq) {
            assert(streamoo_levels_process(&s,samples,240,seq));
            assert(s.result.valid==(seq==7));
        }
        assert(s.result.windows==1 && s.result.rms[0]==0 && s.result.peak[1]==0);
    } else if(test==2) {
        constant(INT32_MIN,INT32_MAX);
        assert(streamoo_levels_init(&s,48000));
        for(unsigned i=0;i<200;++i) assert(streamoo_levels_process(&s,samples,240,i));
        assert(s.result.windows==1 && s.result.valid);
        for(unsigned ch=0;ch<2;++ch) assert(s.result.rms[ch]==32767 && s.result.peak[ch]==32767);
    } else if(test==3) {
        /* Coherent 40ms sine: independent floating reference from exact input. */
        const unsigned frequencies[]={100,1000,8000};
        for(unsigned f=0;f<3;++f) {
            assert(streamoo_levels_init(&s,1920)); double sum[2]={0}; int peak[2]={0};
            for(unsigned block=0;block<8;++block) {
                for(unsigned i=0;i<240;++i) {
                    int l=(int)lround(24000*sin(2*3.141592653589793*frequencies[f]*(block*240+i)/48000));
                    int r=(int)lround(12000*cos(2*3.141592653589793*frequencies[f]*(block*240+i)/48000));
                    samples[2*i]=l*65536; samples[2*i+1]=r*65536;
                    sum[0]+=(double)l*l; sum[1]+=(double)r*r;
                    if(abs(l)>peak[0])peak[0]=abs(l);
                    if(abs(r)>peak[1])peak[1]=abs(r);
                }
                int32_t before[480]; memcpy(before,samples,sizeof samples);
                assert(streamoo_levels_process(&s,samples,240,block));
                assert(!memcmp(before,samples,sizeof samples));
            }
            for(unsigned ch=0;ch<2;++ch) {
                assert(s.result.rms[ch]==(unsigned)floor(sqrt(sum[ch]/1920)));
                assert(s.result.peak[ch]==peak[ch]);
            }
            assert(abs((int)s.result.rms[0]-16970)<=1);
        }
    } else if(test==4) {
        samples[0]=INT32_MIN;
        for(unsigned seq=0;seq<8;++seq) {
            assert(streamoo_levels_process(&s,samples,240,seq)); samples[0]=0;
        }
        assert(s.result.rms[0]==(unsigned)sqrt(1073741824.0/1920));
        assert(s.result.peak[0]==32767 && s.result.rms[1]==0 && s.result.peak[1]==0);
        for(unsigned seq=8;seq<16;++seq) assert(streamoo_levels_process(&s,samples,240,seq));
        assert(s.result.rms[0]==0 && s.result.peak[0]==0 && s.result.windows==2);
    } else if(test==5) {
        constant(10000*65536,-20000*65536);
        for(unsigned seq=0;seq<9;++seq) assert(streamoo_levels_process(&s,samples,240,seq));
        assert(s.result.valid && s.frames==240);
        constant(0,0);
        assert(streamoo_levels_process(&s,samples,240,10)); /* gap */
        assert(!s.result.valid && s.frames==240 && s.result.discontinuities==1);
        assert(s.result.rms[0]==0 && s.result.peak[1]==0);
        for(unsigned seq=11;seq<18;++seq) assert(streamoo_levels_process(&s,samples,240,seq));
        assert(s.result.valid && s.result.windows==2 && s.result.rms[0]==0);
        assert(streamoo_levels_process(&s,samples,240,17)); /* duplicate */
        assert(!s.result.valid && s.result.discontinuities==2 && s.frames==240);
        assert(streamoo_levels_init(&s,1920));
        assert(!s.result.valid && !s.result.windows && !s.have_sequence);
    } else if(test==6) {
        constant(1234*65536,-4321*65536);
        /* Non-divisible window crosses calls; sequence wraps without a gap. */
        assert(streamoo_levels_init(&s,300));
        assert(streamoo_levels_process(&s,samples,239,UINT32_MAX));
        assert(streamoo_levels_process(&s,samples,240,0));
        assert(s.result.windows==1 && s.frames==179 && !s.result.discontinuities);
        assert(s.result.rms[0]==1234 && s.result.rms[1]==4321);
        assert(streamoo_levels_process(&s,samples,240,1));
        assert(s.result.windows==2 && s.frames==119);
        assert(streamoo_levels_init(&s,1));
        assert(streamoo_levels_process(&s,samples,240,0));
        assert(s.result.windows==240 && s.frames==0);
    } else if(test==7) {
        /* Real existing PCM/gain/tap path, not simulated volume independence. */
        static streamoo_pcm_block_t storage[32], taps[2];
        static int32_t output[480], raw[480];
        const uint8_t volumes[]={0,50,100};
        for(unsigned v=0;v<3;++v) {
            streamoo_pcm_ring_t ring,tap; streamoo_pcm_pipeline_t pipe;
            assert(streamoo_pcm_ring_init(&ring,storage,32));
            assert(streamoo_pcm_ring_init(&tap,taps,2));
            assert(streamoo_pcm_pipeline_init(&pipe,&ring,&tap));
            assert(streamoo_levels_init(&s,1920));
            constant(16000*65536,-8000*65536);
            for(unsigned b=0;b<24;++b) assert(streamoo_pcm_push(&ring,samples,b));
            uint32_t gain; assert(streamoo_audio_gain_volume(volumes[v],&gain));
            for(unsigned b=0;b<16;++b) {
                assert(streamoo_pcm_fill(&pipe,output,240,gain));
                uint32_t sequence;
                assert(streamoo_pcm_pop(&tap,raw,&sequence));
                assert(streamoo_levels_process(&s,raw,240,sequence));
                assert(streamoo_pcm_push(&ring,samples,b+24));
            }
            assert(s.result.rms[0]==16000 && s.result.rms[1]==8000);
            assert(s.result.peak[0]==16000 && s.result.peak[1]==8000);
            if(!v) for(unsigned i=0;i<480;++i) assert(output[i]==0);
            if(v==2) assert(output[0]==samples[0]);
        }
    } else if(test==8) {
        /* Quantization is defined toward zero, including asymmetric endpoints. */
        assert(streamoo_levels_init(&s,1));
        constant(-65535,65535);
        assert(streamoo_levels_process(&s,samples,1,0));
        assert(s.result.rms[0]==0 && s.result.rms[1]==0);
        constant(-65536,65536);
        assert(streamoo_levels_process(&s,samples,1,1));
        assert(s.result.rms[0]==1 && s.result.rms[1]==1);
    } else assert(0);
    return 0;
}
