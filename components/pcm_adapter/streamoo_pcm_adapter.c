#include "streamoo_pcm_adapter.h"
static int32_t read16(const uint8_t *p)
{
    uint32_t word=(uint32_t)p[0]|((uint32_t)p[1]<<8);
    int32_t value=word>=32768 ? (int32_t)word-65536 : (int32_t)word;
    return value*65536;
}
bool streamoo_pcm16le_convert(streamoo_pcm_format_t f, const uint8_t *data,
                               size_t bytes, int32_t *lr, size_t frames)
{
    if (!data || !lr || f.rate!=48000 || f.bits!=16 || (f.channels!=1 && f.channels!=2) ||
        frames>240 || bytes!=frames*f.channels*2) return false;
    for (size_t i=0;i<frames;++i) {
        lr[i*2]=read16(data+i*f.channels*2);
        lr[i*2+1]=f.channels==1 ? lr[i*2] : read16(data+i*4+2);
    }
    return true;
}

bool streamoo_pcm_clip_init(streamoo_pcm_clip_t *c, streamoo_pcm_format_t f,
                             const uint8_t *data, size_t bytes)
{
    if (!c || !data || !bytes || f.rate!=48000 || f.bits!=16 || (f.channels!=1 && f.channels!=2) ||
        bytes%(f.channels*2)) return false;
    *c=(streamoo_pcm_clip_t){.format=f,.data=data,.bytes=bytes};
    return true;
}
bool streamoo_pcm_clip_read(streamoo_pcm_clip_t *c,int32_t *lr,size_t frames)
{
    if (!c || !lr || !c->data || !c->bytes || frames>240 || c->cursor>=c->bytes ||
        c->format.rate!=48000 || c->format.bits!=16 || (c->format.channels!=1 && c->format.channels!=2)) return false;
    size_t stride=c->format.channels*2;
    if (c->bytes%stride || c->cursor%stride) return false;
    size_t done=0;
    while (done<frames) {
        size_t count=(c->bytes-c->cursor)/stride;
        if (count>frames-done) count=frames-done;
        if (!streamoo_pcm16le_convert(c->format,c->data+c->cursor,count*stride,lr+done*2,count)) return false;
        c->cursor+=count*stride; done+=count;
        if (c->cursor==c->bytes) c->cursor=0;
    }
    return true;
}
