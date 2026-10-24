#include "audio_tone.h"
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define TONE_IRAM IRAM_ATTR
#define TONE_DRAM DRAM_ATTR
#else
#define TONE_IRAM
#define TONE_DRAM
#endif

/* Original table generated from sin(2*pi*n/48), peak 1036. */
static TONE_DRAM const int16_t wave[48]={
    0,135,268,396,518,631,733,822,897,957,1001,1027,
    1036,1027,1001,957,897,822,733,631,518,396,268,135,
    0,-135,-268,-396,-518,-631,-733,-822,-897,-957,-1001,-1027,
    -1036,-1027,-1001,-957,-897,-822,-733,-631,-518,-396,-268,-135
};

bool TONE_IRAM streamoo_audio_tone_fill(streamoo_audio_tone_t *tone, int32_t *lr, size_t frames)
{
    if (!tone || !lr || frames>STREAMOO_TONE_BLOCK_MAX) return false;
    for (size_t i=0; i<frames; ++i) {
        uint32_t p=tone->position, local=0;
        unsigned channels=0, frequency=1;
        if (p>=48000 && p<144000) { local=p-48000; channels=1; }
        if (p>=192000 && p<288000) { local=p-192000; channels=2; frequency=2; }
        if (p>=336000 && p<432000) { local=p-336000; channels=3; }
        int32_t value=0;
        if (channels) {
            uint32_t envelope=STREAMOO_TONE_RAMP;
            if (local<STREAMOO_TONE_RAMP) envelope=local;
            if (local>=96000-STREAMOO_TONE_RAMP) envelope=95999-local;
            value=((int32_t)wave[(local*frequency)%48]*(int32_t)envelope/(int32_t)STREAMOO_TONE_RAMP)*65536;
        }
        lr[i*2]=(channels&1) ? value : 0;
        lr[i*2+1]=(channels&2) ? value : 0;
        if (tone->position<STREAMOO_TONE_FRAMES) ++tone->position;
    }
    return true;
}
