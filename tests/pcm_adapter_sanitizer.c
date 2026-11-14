#include "streamoo_pcm_adapter.h"
#include <assert.h>
#include <stdint.h>
int main(void)
{
    uint8_t input[960]; int32_t output[482]; output[0]=17; output[481]=19;
    for (unsigned base=0;base<65536;base+=240) {
        for (unsigned i=0;i<240;++i) { unsigned word=(base+i)&65535; input[i*2]=word; input[i*2+1]=word>>8; }
        assert(streamoo_pcm16le_convert((streamoo_pcm_format_t){48000,1,16},input,480,output+1,240));
        assert(output[0]==17 && output[481]==19);
        for (unsigned i=0;i<240;++i) {
            int32_t word=(base+i)&65535; if (word>=32768) word-=65536;
            assert(output[1+i*2]==word*65536 && output[2+i*2]==word*65536);
        }
    }
    assert(!streamoo_pcm16le_convert((streamoo_pcm_format_t){44100,1,16},input,480,output+1,240));
    streamoo_pcm_clip_t clip;
    assert(streamoo_pcm_clip_init(&clip,(streamoo_pcm_format_t){48000,2,16},input,28));
    for (unsigned i=0;i<1000;++i) {
        assert(streamoo_pcm_clip_read(&clip,output+1,240));
        assert(output[0]==17 && output[481]==19);
        assert(clip.cursor<28 && clip.cursor%4==0);
    }
    return 0;
}
