#include "streamoo_decode_stream.h"
#include <string.h>

bool streamoo_decode_stream_init(streamoo_decode_stream_t *s,const uint8_t *file,size_t bytes,
    void *context,streamoo_stream_decode_fn decode,streamoo_stream_reset_fn reset,
    size_t min_frames,size_t max_frames)
{
    if (!s || !file || !bytes || !decode || !reset || !min_frames || max_frames<min_frames ||
        max_frames>UINT32_MAX) return false;
    *s=(streamoo_decode_stream_t){.file=file,.bytes=bytes,.context=context,.decode=decode,
        .reset=reset,.min_frames=min_frames,.max_frames=max_frames};
    return true;
}
static bool next_packet(streamoo_decode_stream_t *s)
{
    /* Bound work even if a backend consumes bytes but never emits PCM. */
    for (unsigned work=0;work<10000;++work) {
        if (!s->pending) {
            if (s->offset==s->bytes) {
                if (s->cycle_frames<s->min_frames || s->cycle_frames>s->max_frames ||
                    s->reset(s->context)!=ESP_OK) return false;
                s->last_cycle_frames=s->cycle_frames;
                ++s->cycles; s->cycle_frames=0; s->offset=0;
            }
            s->pending=s->bytes-s->offset;
            if (s->pending>sizeof s->input) s->pending=sizeof s->input;
            memcpy(s->input,s->file+s->offset,s->pending);
            s->offset+=s->pending; s->input_cursor=0;
        }
        streamoo_decode_result_t r={0};
        esp_err_t error=s->decode(s->context,s->input+s->input_cursor,s->pending,
            s->offset==s->bytes,s->pcm,sizeof s->pcm,&r);
        ++s->calls;
        if (error!=ESP_OK || r.consumed>s->pending || r.produced>sizeof s->pcm ||
            (!r.consumed && !r.produced)) return false;
        s->pending-=r.consumed; s->input_cursor+=r.consumed;
        if (r.produced) {
            if (r.format.rate!=48000 || r.format.channels!=2 || r.format.bits!=16 ||
                r.produced%4 || r.produced/4>s->max_frames-s->cycle_frames) return false;
            s->cycle_frames+=r.produced/4; s->format=r.format;
            s->pcm_bytes=r.produced; s->pcm_cursor=0;
            return true;
        }
    }
    return false;
}
bool streamoo_decode_stream_read(streamoo_decode_stream_t *s,int32_t *lr,size_t frames)
{
    if (!s || !lr || !s->decode || s->failed || !frames || frames>240) return false;
    size_t done=0;
    while (done<frames) {
        if (s->pcm_cursor==s->pcm_bytes && !next_packet(s)) { s->failed=true; return false; }
        size_t count=(s->pcm_bytes-s->pcm_cursor)/4;
        if (count>frames-done) count=frames-done;
        if (!streamoo_pcm16le_convert(s->format,s->pcm+s->pcm_cursor,count*4,lr+done*2,count)) {
            s->failed=true; return false;
        }
        done+=count; s->pcm_cursor+=count*4;
    }
    return true;
}
