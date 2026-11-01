#include "streamoo_flac_alt.h"
#include "streamoo_flac_parse.h"
#include "streamoo_flac_header.h"
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_OGG
#define DR_FLAC_NO_SIMD
#define DR_FLAC_IMPLEMENTATION
#include "third_party/dr_flac/dr_flac.h"

#define FRAME_CACHE (STREAMOO_FLAC_MAX_FRAME_BYTES+16u+4096u)
struct streamoo_flac_alt {
    drflac *decoder;
    esp_es_parse_frame_info_t info;
    bool bos,opened;
    uint8_t metadata[42],cache[FRAME_CACHE];
    size_t bytes,source_pos,source_header,source_frame;
};
static void *allocate(size_t n,void *context) {
    (void)context;
#ifdef ESP_PLATFORM
    return heap_caps_malloc(n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    return malloc(n);
#endif
}
static void *resize(void *p,size_t n,void *context) {
    (void)context;
#ifdef ESP_PLATFORM
    return heap_caps_realloc(p,n,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    return realloc(p,n);
#endif
}
static void release(void *p,void *context) {(void)context;free(p);}
static const drflac_allocation_callbacks allocator={NULL,allocate,resize,release};
static size_t read_frame(void *context,void *out,size_t n) {
    streamoo_flac_alt_t *s=context;
    size_t available=s->source_header+s->source_frame-s->source_pos;
    if(n>available)n=available;
    size_t done=0;
    if(s->source_pos<s->source_header){
        done=s->source_header-s->source_pos;if(done>n)done=n;
        memcpy(out,s->metadata+s->source_pos,done);s->source_pos+=done;
    }
    if(done<n){memcpy((uint8_t *)out+done,s->cache+s->source_pos-s->source_header,n-done);s->source_pos+=n-done;}
    return n;
}
static drflac_bool32 seek_frame(void *context,int offset,drflac_seek_origin origin) {
    streamoo_flac_alt_t *s=context;
    if(s->opened||origin==DRFLAC_SEEK_END)return DRFLAC_FALSE;
    int64_t pos=(origin==DRFLAC_SEEK_CUR?(int64_t)s->source_pos:0)+offset;
    if(pos<0||(uint64_t)pos>s->source_header+s->source_frame)return DRFLAC_FALSE;
    s->source_pos=(size_t)pos;return DRFLAC_TRUE;
}
static unsigned block_frames(const uint8_t *p) {
    unsigned at=5,count=0;
    if(p[4]&128){for(unsigned mask=128;mask&&(p[4]&mask);mask>>=1)++count;at=4+count;}
    unsigned code=p[2]>>4;
    if(code==6)return (unsigned)p[at]+1;
    if(code==7)return ((unsigned)p[at]<<8)+p[at+1]+1;
    return code==1?192:code<=5?144u<<code:1u<<code;
}
static esp_err_t decode_frame(streamoo_flac_alt_t *s,uint8_t *pcm,size_t cap,streamoo_decode_result_t *r) {
    unsigned frames=block_frames(s->cache),channels=s->info.aud_info.channel;
    r->needed=(size_t)frames*channels*4;
    if(r->needed>cap)return ESP_ERR_INVALID_SIZE;
    s->source_pos=0;s->source_frame=s->info.frame_size;s->source_header=s->decoder?0:42;
    if(!s->decoder){
        s->decoder=drflac_open(read_frame,seek_frame,NULL,s,&allocator);
        if(!s->decoder){r->codec_status=-1002;return ESP_FAIL;}
        s->opened=true;
    }
    drflac_uint64 got=drflac_read_pcm_frames_s32(s->decoder,frames,(drflac_int32 *)pcm);
    if(got!=frames||s->source_pos!=s->source_header+s->source_frame){
        r->codec_status=-1003;return ESP_ERR_INVALID_RESPONSE;
    }
    r->format=(streamoo_pcm_format_t){s->decoder->sampleRate,s->decoder->channels,32};
    r->produced=(size_t)got*channels*4;r->needed=0;
    return ESP_OK;
}
/* dr_flac's pull API must never mistake a temporary empty source for EOF.
 * Feed complete CRC-checked frames to one persistent decoder, without I/O. */
static int packet(streamoo_flac_alt_t *s,bool eos) {
    while(s->bytes){
        esp_es_parse_raw_t raw={.buffer=s->cache,.len=s->bytes,.bos=s->bos,.eos=eos};
        esp_es_parse_err_t status=streamoo_flac_parse(&raw,&s->info);
        if(status==ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH)return 0;
        if(status!=ESP_ES_PARSE_ERR_OK)return -1;
        if(s->info.skipped_size){
            if(s->bos){memcpy(s->metadata,s->cache,42);s->metadata[4]=128;s->bos=false;}
            size_t used=s->info.skipped_size;
            memmove(s->cache,s->cache+used,s->bytes-used);s->bytes-=used;continue;
        }
        return s->info.frame_size==ES_PARSE_REPARSE_FRAME_SIZE?0:1;
    }
    return 0;
}
esp_err_t streamoo_flac_alt_open(streamoo_flac_alt_t **out) {
    *out=allocate(sizeof **out,NULL);if(!*out)return ESP_ERR_NO_MEM;
    memset(*out,0,sizeof **out);(*out)->bos=true;return ESP_OK;
}
esp_err_t streamoo_flac_alt_process(streamoo_flac_alt_t *s,uint8_t *p,size_t n,bool eos,
    uint8_t *pcm,size_t cap,streamoo_decode_result_t *r) {
    *r=(streamoo_decode_result_t){0};
    int ready=packet(s,false);
    if(!ready){
        size_t take=n;if(eos&&take>1)--take; /* Keep final byte pending until cached frames drain. */
        if(take>FRAME_CACHE-s->bytes)take=FRAME_CACHE-s->bytes;
        if(!take){r->codec_status=-1004;return ESP_ERR_INVALID_SIZE;}
        memcpy(s->cache+s->bytes,p,take);s->bytes+=take;r->consumed=take;
        ready=packet(s,eos&&take==n);
    }
    if(ready<0){r->codec_status=-1001;return ESP_ERR_INVALID_RESPONSE;}
    if(!ready)return ESP_OK;
    esp_err_t error=decode_frame(s,pcm,cap,r);
    if(error==ESP_OK){size_t used=s->info.frame_size;memmove(s->cache,s->cache+used,s->bytes-used);s->bytes-=used;}
    return error;
}
esp_err_t streamoo_flac_alt_reset(streamoo_flac_alt_t *s) {
    if(s->decoder)drflac_close(s->decoder);
    free(s->info.extra_data);memset(s,0,sizeof *s);s->bos=true;return ESP_OK;
}
void streamoo_flac_alt_close(streamoo_flac_alt_t *s) {
    if(s){streamoo_flac_alt_reset(s);free(s);}
}
