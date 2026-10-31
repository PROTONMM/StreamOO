#include "streamoo_audio_gain.h"
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define GAIN_IRAM IRAM_ATTR
#define GAIN_DRAM DRAM_ATTR
#else
#define GAIN_IRAM
#define GAIN_DRAM
#endif

/* Original table: round(65536 * (v/100)^2). No runtime float/pow. */
static GAIN_DRAM const uint32_t volume_gain[101]={
    0,7,26,59,105,164,236,321,419,531,
    655,793,944,1108,1285,1475,1678,1894,2123,2366,
    2621,2890,3172,3467,3775,4096,4430,4778,5138,5512,
    5898,6298,6711,7137,7576,8028,8493,8972,9463,9968,
    10486,11017,11561,12118,12688,13271,13867,14477,15099,15735,
    16384,17046,17721,18409,19110,19825,20552,21293,22046,22813,
    23593,24386,25192,26011,26844,27689,28547,29419,30304,31202,
    32113,33037,33974,34924,35888,36864,37854,38856,39872,40901,
    41943,42998,44066,45148,46242,47350,48470,49604,50751,51911,
    53084,54270,55470,56682,57908,59146,60398,61663,62941,64232,
    65536
};
void streamoo_audio_gain_init(streamoo_audio_gain_t *s)
{
    if (s) *s=(streamoo_audio_gain_t){0};
}
bool streamoo_audio_gain_volume(uint8_t volume, uint32_t *out)
{
    if (!out || volume>100) return false;
    *out=volume_gain[volume];
    return true;
}
bool GAIN_IRAM streamoo_audio_gain_target(streamoo_audio_gain_t *s, uint32_t target)
{
    if (!s || target>STREAMOO_GAIN_UNITY) return false;
    if ((uint32_t)s->target==target) return true;
    s->target=(int32_t)target;
    int32_t delta=s->target-s->current;
    s->step=delta/(int32_t)STREAMOO_GAIN_RAMP_FRAMES;
    s->remainder=delta%(int32_t)STREAMOO_GAIN_RAMP_FRAMES;
    s->error=0;
    s->remaining=STREAMOO_GAIN_RAMP_FRAMES;
    return true;
}
bool GAIN_IRAM streamoo_audio_gain_process(streamoo_audio_gain_t *s, int32_t *lr, size_t frames)
{
    if (!s || !lr || frames>STREAMOO_GAIN_BLOCK_MAX) return false;
    for (size_t i=0;i<frames;++i) {
        if (s->remaining) {
            s->current+=s->step;
            s->error+=s->remainder;
            if (s->error>=(int32_t)STREAMOO_GAIN_RAMP_FRAMES) {
                ++s->current; s->error-=(int32_t)STREAMOO_GAIN_RAMP_FRAMES;
            } else if (s->error<=-(int32_t)STREAMOO_GAIN_RAMP_FRAMES) {
                --s->current; s->error+=(int32_t)STREAMOO_GAIN_RAMP_FRAMES;
            }
            if (!--s->remaining) s->current=s->target;
        }
        /* Signed division rounds toward zero; no negative signed shift/overflow. */
        lr[i*2]=(int32_t)((int64_t)lr[i*2]*s->current/65536);
        lr[i*2+1]=(int32_t)((int64_t)lr[i*2+1]*s->current/65536);
    }
    return true;
}
