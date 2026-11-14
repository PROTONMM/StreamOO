#include "streamoo_pcm.h"
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define PCM_IRAM IRAM_ATTR
#else
#define PCM_IRAM
#endif
_Static_assert(STREAMOO_PCM_LOW_BLOCKS*STREAMOO_PCM_FRAMES==STREAMOO_GAIN_RAMP_FRAMES,"Reserve must cover ramp");
/* IDF disables hardware RMW atomics globally. We use only aligned loads and
 * stores; verify their native memw/l32i/s32i expansion in the pinned ELF. */
_Static_assert(sizeof(atomic_uint)==4 && _Alignof(atomic_uint)>=4,"PCM requires aligned 32-bit counters");
bool streamoo_pcm_ring_init(streamoo_pcm_ring_t *r, streamoo_pcm_block_t *storage, uint32_t capacity)
{
    if (!r || !storage || capacity<2 || capacity>64 || (capacity&(capacity-1))) return false;
    r->storage=storage; r->capacity=capacity;
    atomic_init(&r->read,0); atomic_init(&r->write,0);
    return true;
}
uint32_t PCM_IRAM streamoo_pcm_count(const streamoo_pcm_ring_t *r)
{
    if (!r) return 0;
    uint32_t w=atomic_load_explicit(&r->write,memory_order_acquire);
    uint32_t n=w-atomic_load_explicit(&r->read,memory_order_acquire);
    return n>r->capacity ? r->capacity : n;
}
bool PCM_IRAM streamoo_pcm_push(streamoo_pcm_ring_t *r, const int32_t *samples, uint32_t sequence)
{
    if (!r || !samples) return false;
    uint32_t w=atomic_load_explicit(&r->write,memory_order_relaxed);
    uint32_t read=atomic_load_explicit(&r->read,memory_order_acquire);
    if (w-read>=r->capacity) return false;
    int32_t *dst=r->storage[w&(r->capacity-1)].samples;
    for (unsigned i=0;i<STREAMOO_PCM_WORDS;++i) dst[i]=samples[i];
    r->storage[w&(r->capacity-1)].sequence=sequence;
    atomic_store_explicit(&r->write,w+1,memory_order_release);
    return true;
}
bool PCM_IRAM streamoo_pcm_pop(streamoo_pcm_ring_t *r, int32_t *samples, uint32_t *sequence)
{
    if (!r || !samples) return false;
    uint32_t read=atomic_load_explicit(&r->read,memory_order_relaxed);
    uint32_t w=atomic_load_explicit(&r->write,memory_order_acquire);
    if (read==w) return false;
    const int32_t *src=r->storage[read&(r->capacity-1)].samples;
    for (unsigned i=0;i<STREAMOO_PCM_WORDS;++i) samples[i]=src[i];
    if (sequence) *sequence=r->storage[read&(r->capacity-1)].sequence;
    atomic_store_explicit(&r->read,read+1,memory_order_release);
    return true;
}
bool streamoo_pcm_pipeline_init(streamoo_pcm_pipeline_t *p, streamoo_pcm_ring_t *source, streamoo_pcm_ring_t *tap)
{
    if (!p || !source || source==tap || source->capacity<STREAMOO_PCM_START_BLOCKS) return false;
    *p=(streamoo_pcm_pipeline_t){.source=source,.tap=tap};
    streamoo_audio_gain_init(&p->gain);
    return true;
}
bool PCM_IRAM streamoo_pcm_fill(streamoo_pcm_pipeline_t *p, int32_t *lr, size_t frames, uint32_t target)
{
    if (!p || !lr || frames!=STREAMOO_PCM_FRAMES || target>STREAMOO_GAIN_UNITY) return false;
    uint32_t available=streamoo_pcm_count(p->source);
    if (p->stats.mode==STREAMOO_PCM_BUFFERING && available>=STREAMOO_PCM_START_BLOCKS) {
        p->stats.mode=STREAMOO_PCM_ACTIVE; p->hard_seen=false; ++p->stats.restarts;
    }
    if (p->stats.mode==STREAMOO_PCM_ACTIVE && available<=STREAMOO_PCM_LOW_BLOCKS) {
        p->stats.mode=STREAMOO_PCM_FADING; ++p->stats.underflows;
    }
    bool received=false;
    if (p->stats.mode!=STREAMOO_PCM_BUFFERING) received=streamoo_pcm_pop(p->source,lr,NULL);
    if (received) {
        ++p->stats.received;
        p->last[0]=lr[STREAMOO_PCM_WORDS-2]; p->last[1]=lr[STREAMOO_PCM_WORDS-1];
    } else {
        ++p->stats.zero_blocks;
        for (unsigned i=0;i<STREAMOO_PCM_WORDS;++i) lr[i]=0;
    }
    /* Copy exact raw input (or zeros for missing input), before gain changes it. */
    if (p->tap && !streamoo_pcm_push(p->tap,lr,p->stats.output_blocks)) ++p->stats.tap_drops;
    ++p->stats.output_blocks;
    uint32_t desired=p->stats.mode==STREAMOO_PCM_ACTIVE ? target : 0;
    streamoo_audio_gain_target(&p->gain,desired);
    if (!received && p->stats.mode==STREAMOO_PCM_FADING) {
        if (!p->hard_seen) { p->hard_seen=true; ++p->stats.hard_underflows; }
        for (unsigned i=0;i<STREAMOO_PCM_FRAMES;++i) { lr[i*2]=p->last[0]; lr[i*2+1]=p->last[1]; }
    }
    streamoo_audio_gain_process(&p->gain,lr,frames);
    if (p->stats.mode==STREAMOO_PCM_FADING && !p->gain.remaining && !p->gain.current)
        p->stats.mode=STREAMOO_PCM_BUFFERING;
    p->stats.queued=streamoo_pcm_count(p->source);
    return true;
}
