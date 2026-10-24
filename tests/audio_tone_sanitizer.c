#include "audio_tone.h"
#include <assert.h>
#include <stdint.h>
int main(void)
{
    streamoo_audio_tone_t tone={0};
    int32_t data[482]; data[0]=123; data[481]=321;
    for (unsigned block=0;block<2100;++block) {
        assert(streamoo_audio_tone_fill(&tone,data+1,240));
        assert(data[0]==123 && data[481]==321);
        for (unsigned i=1;i<=480;++i) {
            assert(data[i]>=-1036*65536 && data[i]<=1036*65536);
            if (block>=2000) assert(data[i]==0);
        }
    }
    assert(tone.position==STREAMOO_TONE_FRAMES);
    assert(!streamoo_audio_tone_fill(&tone,data+1,241));
    return 0;
}
