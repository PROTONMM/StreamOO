#include "streamoo_audio_gain.h"
#include "audio_tone.h"
#include "audio_control_model.h"
#include <assert.h>
#include <limits.h>
int main(void)
{
    streamoo_audio_gain_t gain; streamoo_audio_gain_init(&gain);
    uint32_t phase=0;
    int32_t buffer[482]; buffer[0]=17; buffer[481]=19;
    for (unsigned n=0;n<2000;++n) {
        uint32_t target;
        assert(streamoo_audio_gain_volume(n%101,&target));
        assert(streamoo_audio_gain_target(&gain,n%4 ? target : 0));
        assert(streamoo_audio_sine_fill(&phase,buffer+1,240));
        assert(streamoo_audio_gain_process(&gain,buffer+1,240));
        assert(buffer[0]==17 && buffer[481]==19);
        assert(gain.current>=0 && gain.current<=65536);
        for (unsigned i=1;i<=480;++i) assert(buffer[i]>=-1036*65536 && buffer[i]<=1036*65536);
    }
    streamoo_audio_control_model_t model;
    assert(streamoo_audio_control_model_init(&model,50));
    const streamoo_control_command_t changes[]={{STREAMOO_CONTROL_VOLUME_DELTA,INT_MAX},
        {STREAMOO_CONTROL_TOGGLE,0},{STREAMOO_CONTROL_VOLUME_DELTA,INT_MIN},{STREAMOO_CONTROL_TOGGLE,0}};
    for (unsigned i=0;i<4;++i) {
        assert(streamoo_audio_control_model_apply(&model,&changes[i]));
        uint32_t target; assert(streamoo_audio_control_model_gain(&model,&target));
        assert(target<=65536);
    }
    assert(streamoo_audio_gain_target(&gain,65536));
    for (unsigned i=0;i<10;++i) {
        for (unsigned j=1;j<=480;++j) buffer[j]=j&1 ? INT_MIN : INT_MAX;
        assert(streamoo_audio_gain_process(&gain,buffer+1,240));
    }
    buffer[1]=INT_MIN; buffer[2]=INT_MAX;
    assert(streamoo_audio_gain_process(&gain,buffer+1,1));
    assert(buffer[1]==INT_MIN && buffer[2]==INT_MAX);
    return 0;
}
