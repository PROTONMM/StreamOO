#include "streamoo_pcm.h"
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>

static streamoo_pcm_block_t storage[32], taps[4], second[32];
static streamoo_pcm_ring_t ring,tap,other;
static int32_t input[480],output[480],raw[480],mirror[480];
static void pattern(unsigned tag)
{ for (unsigned i=0;i<480;++i) input[i]=(int32_t)(tag*1000+i); }
static void constant(void)
{ for (unsigned i=0;i<240;++i) { input[i*2]=1000000; input[i*2+1]=-500000; } }
static void init(void)
{
    assert(streamoo_pcm_ring_init(&ring,storage,32));
    assert(streamoo_pcm_ring_init(&tap,taps,4));
}
static void prefill(unsigned count)
{ for (unsigned i=0;i<count;++i) assert(streamoo_pcm_push(&ring,input,i)); }
static void *writer(void *unused)
{
    (void)unused; int32_t block[480];
    for (unsigned n=0;n<20000;++n) {
        for (unsigned i=0;i<480;++i) block[i]=(int32_t)(n*1000+i);
        while (!streamoo_pcm_push(&ring,block,n)) sched_yield();
    }
    return NULL;
}
static void *reader(void *unused)
{
    (void)unused; int32_t block[480]; uint32_t seq;
    for (unsigned n=0;n<20000;++n) {
        while (!streamoo_pcm_pop(&ring,block,&seq)) sched_yield();
        assert(seq==n);
        for (unsigned i=0;i<480;++i) assert(block[i]==(int32_t)(n*1000+i));
    }
    return NULL;
}
int main(int argc,char **argv)
{
    assert(argc==2); int test=atoi(argv[1]); init();
    streamoo_pcm_pipeline_t pipe;
    assert(streamoo_pcm_pipeline_init(&pipe,&ring,&tap));
    if (test==0) {
        assert(!streamoo_pcm_ring_init(&ring,storage,3));
        assert(!streamoo_pcm_ring_init(&ring,NULL,32));
        assert(!streamoo_pcm_ring_init(NULL,storage,32));
        assert(!streamoo_pcm_pipeline_init(NULL,&ring,&tap));
        assert(!streamoo_pcm_pipeline_init(&pipe,&ring,&ring));
        streamoo_pcm_ring_t small; assert(streamoo_pcm_ring_init(&small,taps,4));
        assert(!streamoo_pcm_pipeline_init(&pipe,&small,NULL));
    } else if (test==1) {
        uint32_t sequence=99;
        assert(!streamoo_pcm_pop(&ring,output,&sequence)); assert(sequence==99);
        for (unsigned n=0;n<32;++n) { pattern(n); assert(streamoo_pcm_push(&ring,input,n)); }
        pattern(99); assert(!streamoo_pcm_push(&ring,input,99));
        for (unsigned n=0;n<32;++n) {
            assert(streamoo_pcm_pop(&ring,output,&sequence)); assert(sequence==n);
            for (unsigned i=0;i<480;++i) assert(output[i]==(int32_t)(n*1000+i));
        }
        assert(!streamoo_pcm_pop(&ring,output,NULL));
    } else if (test==2) {
        atomic_store(&ring.read,UINT_MAX-4); atomic_store(&ring.write,UINT_MAX-4);
        for (unsigned n=0;n<1000;++n) {
            pattern(n); assert(streamoo_pcm_push(&ring,input,n));
            assert(streamoo_pcm_count(&ring)==1);
            uint32_t seq; assert(streamoo_pcm_pop(&ring,output,&seq)); assert(seq==n);
            assert(!memcmp(input,output,sizeof input)); assert(streamoo_pcm_count(&ring)==0);
        }
    } else if (test==3) {
        pthread_t a,b; assert(!pthread_create(&a,NULL,writer,NULL)); assert(!pthread_create(&b,NULL,reader,NULL));
        assert(!pthread_join(a,NULL)); assert(!pthread_join(b,NULL)); assert(streamoo_pcm_count(&ring)==0);
    } else if (test==4) {
        constant(); prefill(32);
        const uint32_t gains[]={0,2072,65536};
        for (unsigned n=0;n<36;++n) {
            assert(streamoo_pcm_fill(&pipe,output,240,gains[n/12]));
            uint32_t seq; assert(streamoo_pcm_pop(&tap,raw,&seq)); assert(seq==n);
            assert(!memcmp(raw,input,sizeof input));
            assert(streamoo_pcm_push(&ring,input,n));
        }
        assert(pipe.stats.tap_drops==0 && pipe.stats.underflows==0);
    } else if (test==5) {
        constant(); prefill(32);
        for (unsigned n=0;n<22;++n) assert(streamoo_pcm_fill(&pipe,output,240,65536));
        assert(pipe.gain.current==65536 && streamoo_pcm_count(&ring)==10);
        int32_t last=1000000;
        for (unsigned n=0;n<10;++n) {
            assert(streamoo_pcm_fill(&pipe,output,240,65536));
            for (unsigned i=0;i<240;++i) {
                assert(output[i*2]<=last && last-output[i*2]<=500); last=output[i*2];
            }
        }
        assert(last==0 && pipe.stats.mode==STREAMOO_PCM_BUFFERING);
        assert(pipe.stats.underflows==1 && pipe.stats.hard_underflows==0);
        assert(streamoo_pcm_fill(&pipe,output,240,65536));
        for (unsigned i=0;i<480;++i) assert(output[i]==0);
        prefill(23); assert(streamoo_pcm_fill(&pipe,output,240,65536));
        assert(streamoo_pcm_count(&ring)==23 && pipe.stats.mode==STREAMOO_PCM_BUFFERING);
        assert(streamoo_pcm_push(&ring,input,23));
        for (unsigned n=0;n<10;++n) assert(streamoo_pcm_fill(&pipe,output,240,65536));
        assert(pipe.stats.mode==STREAMOO_PCM_ACTIVE && pipe.gain.current==65536);
        assert(pipe.stats.restarts==2 && pipe.stats.underflows==1);
    } else if (test==6) {
        constant(); prefill(32);
        for (unsigned n=0;n<16;++n) {
            assert(streamoo_pcm_fill(&pipe,output,240,0));
            for (unsigned i=0;i<480;++i) assert(output[i]==0);
        }
        assert(pipe.stats.received==16 && streamoo_pcm_count(&ring)==16);
    } else if (test==7) {
        constant(); prefill(32); assert(streamoo_pcm_ring_init(&other,second,32));
        for (unsigned n=0;n<32;++n) assert(streamoo_pcm_push(&other,input,n));
        streamoo_pcm_pipeline_t without_tap; assert(streamoo_pcm_pipeline_init(&without_tap,&other,NULL));
        for (unsigned n=0;n<100;++n) {
            unsigned target=n%20<10 ? 65536 : 2072;
            assert(streamoo_pcm_fill(&pipe,output,240,target));
            assert(streamoo_pcm_fill(&without_tap,mirror,240,target));
            assert(!memcmp(output,mirror,sizeof output));
            assert(streamoo_pcm_push(&ring,input,n)); assert(streamoo_pcm_push(&other,input,n));
        }
        assert(pipe.stats.tap_drops==96 && pipe.stats.underflows==0);
        uint32_t seq; for (unsigned n=0;n<4;++n) { assert(streamoo_pcm_pop(&tap,raw,&seq)); assert(seq==n); }
        assert(streamoo_pcm_fill(&pipe,output,240,65536));
        assert(streamoo_pcm_pop(&tap,raw,&seq)); assert(seq==100); /* gap observable */
        pipe.stats.output_blocks=UINT_MAX-1;
        for (unsigned n=0;n<3;++n) {
            assert(streamoo_pcm_fill(&pipe,output,240,65536));
            assert(streamoo_pcm_pop(&tap,raw,&seq)); assert(seq==(uint32_t)(UINT_MAX-1+n));
            assert(streamoo_pcm_push(&ring,input,n));
        }
    } else if (test==8) {
        memset(output,0x5a,sizeof output); streamoo_pcm_pipeline_t before=pipe;
        assert(!streamoo_pcm_fill(&pipe,output,239,65536));
        assert(!streamoo_pcm_fill(&pipe,output,240,65537));
        assert(!memcmp(&before,&pipe,sizeof pipe));
        for (unsigned i=0;i<480;++i) assert(output[i]==0x5a5a5a5a);
    } else if (test==9) {
        constant(); prefill(23);
        assert(streamoo_pcm_fill(&pipe,output,240,65536)); assert(pipe.stats.mode==STREAMOO_PCM_BUFFERING);
        assert(streamoo_pcm_count(&ring)==23);
        assert(streamoo_pcm_push(&ring,input,23)); assert(streamoo_pcm_fill(&pipe,output,240,65536));
        assert(pipe.stats.mode==STREAMOO_PCM_ACTIVE && output[0]>0 && output[0]<1000000);
    } else {
        pipe.stats.mode=STREAMOO_PCM_ACTIVE;
        pipe.last[0]=1000000; pipe.last[1]=-500000;
        pipe.gain.current=pipe.gain.target=65536;
        for (unsigned n=0;n<10;++n) assert(streamoo_pcm_fill(&pipe,output,240,65536));
        assert(pipe.stats.hard_underflows==1 && pipe.gain.current==0);
        assert(pipe.stats.mode==STREAMOO_PCM_BUFFERING);
        uint32_t seq; assert(streamoo_pcm_pop(&tap,raw,&seq));
        for (unsigned i=0;i<480;++i) assert(raw[i]==0); /* never analyze held fallback */
    }
    return 0;
}
