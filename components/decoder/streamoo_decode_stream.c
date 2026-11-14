#include "streamoo_decode_stream.h"
#include <string.h>

bool streamoo_decode_stream_init(streamoo_decode_stream_t *s,streamoo_byte_source_t source,
    void *context,streamoo_stream_decode_fn decode,streamoo_stream_reset_fn reset,
    size_t min_frames,size_t max_frames)
{
    if (!s || !source.read || !source.rewind || !decode || !reset || !min_frames ||
        max_frames<min_frames || max_frames>UINT32_MAX) return false;
    *s=(streamoo_decode_stream_t){.source=source,.context=context,.decode=decode,
        .reset=reset,.min_frames=min_frames,.max_frames=max_frames};
    return true;
}
static streamoo_stream_result_t next_packet(streamoo_decode_stream_t *s)
{
    for (unsigned work=0;work<10000;++work) {
        if (!s->pending) {
            if (s->input_eof) {
                if (s->cycle_frames<s->min_frames || s->cycle_frames>s->max_frames ||
                    s->reset(s->context)!=ESP_OK || !s->source.rewind(s->source.context))
                    return STREAMOO_STREAM_FAILED;
                s->last_cycle_frames=s->cycle_frames;
                ++s->cycles; s->cycle_frames=0; s->input_eof=false;
            }
            size_t bytes=0; bool last=false;
            streamoo_source_result_t result=s->source.read(s->source.context,s->input,
                sizeof s->input,&bytes,&last);
            if (result==STREAMOO_SOURCE_DATA) {
                if (!bytes || bytes>sizeof s->input) return STREAMOO_STREAM_FAILED;
                s->pending=bytes; s->input_cursor=0; s->input_eof=last;
            } else {
                if (bytes || last) return STREAMOO_STREAM_FAILED;
                if (result==STREAMOO_SOURCE_WAIT) return STREAMOO_STREAM_WAIT;
                /* Valid nonempty sources signal EOF on their final DATA. */
                return STREAMOO_STREAM_FAILED;
            }
        }
        streamoo_decode_result_t r={0};
        esp_err_t error=s->decode(s->context,s->input+s->input_cursor,s->pending,
            s->input_eof,s->pcm,sizeof s->pcm,&r);
        ++s->calls;
        if (error!=ESP_OK || r.consumed>s->pending || r.produced>sizeof s->pcm ||
            (!r.consumed && !r.produced)) return STREAMOO_STREAM_FAILED;
        s->pending-=r.consumed; s->input_cursor+=r.consumed;
        if (r.produced) {
            if (r.format.rate!=48000 || r.format.channels!=2 || r.format.bits!=16 ||
                r.produced%4 || r.produced/4>s->max_frames-s->cycle_frames)
                return STREAMOO_STREAM_FAILED;
            s->cycle_frames+=r.produced/4; s->format=r.format;
            s->pcm_bytes=r.produced; s->pcm_cursor=0; return STREAMOO_STREAM_READY;
        }
    }
    return STREAMOO_STREAM_FAILED;
}
streamoo_stream_result_t streamoo_decode_stream_read(streamoo_decode_stream_t *s,int32_t *lr,size_t frames)
{
    if (!s || !lr || !s->decode || s->failed || !frames || frames>240)
        return STREAMOO_STREAM_FAILED;
    if (s->requested && s->requested!=frames) { s->failed=true; return STREAMOO_STREAM_FAILED; }
    s->requested=frames;
    while (s->assembled<frames) {
        if (s->pcm_cursor==s->pcm_bytes) {
            streamoo_stream_result_t result=next_packet(s);
            if (result!=STREAMOO_STREAM_READY) {
                if (result==STREAMOO_STREAM_FAILED) s->failed=true;
                return result;
            }
        }
        size_t count=(s->pcm_bytes-s->pcm_cursor)/4;
        if (count>frames-s->assembled) count=frames-s->assembled;
        if (!streamoo_pcm16le_convert(s->format,s->pcm+s->pcm_cursor,count*4,
            s->block+s->assembled*2,count)) { s->failed=true; return STREAMOO_STREAM_FAILED; }
        s->assembled+=count; s->pcm_cursor+=count*4;
    }
    memcpy(lr,s->block,frames*2*sizeof(*lr));
    s->assembled=0; s->requested=0; return STREAMOO_STREAM_READY;
}
