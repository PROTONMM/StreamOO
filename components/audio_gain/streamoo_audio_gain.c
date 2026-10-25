#include "streamoo_audio_gain.h"
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define GAIN_IRAM IRAM_ATTR
#define GAIN_DRAM DRAM_ATTR
#else
#define GAIN_IRAM
#define GAIN_DRAM
#endif

/* Original table: round(65536 * 10^((60*(v/100-1))/20)), v>0. */
static GAIN_DRAM const uint32_t volume_gain[101]={
    0,70,75,81,86,93,99,106,114,122,
    131,140,150,161,172,185,198,212,227,243,
    261,280,300,321,344,369,395,423,453,486,
    521,558,598,640,686,735,788,844,905,969,
    1039,1113,1193,1278,1369,1467,1572,1685,1805,1934,
    2072,2221,2379,2550,2732,2927,3137,3361,3601,3859,
    4135,4431,4748,5087,5451,5841,6259,6706,7186,7700,
    8250,8841,9473,10150,10876,11654,12488,13381,14338,15363,
    16462,17639,18901,20253,21701,23253,24916,26698,28608,30653,
    32846,35195,37712,40409,43299,46396,49714,53270,57079,61162,
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
