#include "streamoo_byte_source.h"
#include <string.h>
bool streamoo_byte_ring_init(streamoo_byte_ring_t *r,uint8_t *data,size_t capacity)
{
    if (!r || !data || capacity<2) return false;
    *r=(streamoo_byte_ring_t){.data=data,.capacity=capacity}; return true;
}
size_t streamoo_byte_ring_write(streamoo_byte_ring_t *r,const uint8_t *data,size_t bytes)
{
    if (!r || !r->data || !data || r->ended || r->failed) return 0;
    if (bytes>r->capacity-r->count) bytes=r->capacity-r->count;
    size_t tail=(r->head+r->count)%r->capacity;
    size_t first=r->capacity-tail; if (first>bytes) first=bytes;
    memcpy(r->data+tail,data,first); memcpy(r->data,data+first,bytes-first);
    r->count+=bytes; return bytes;
}
void streamoo_byte_ring_finish(streamoo_byte_ring_t *r,bool failed)
{
    if (r) { r->ended=true; r->failed|=failed; }
}
streamoo_source_result_t streamoo_byte_ring_read(void *context,uint8_t *out,size_t capacity,
    size_t *bytes,bool *last)
{
    if (bytes) *bytes=0;
    if (last) *last=false;
    streamoo_byte_ring_t *r=context;
    if (!r || !r->data || !out || !capacity || !bytes || !last || r->failed)
        return STREAMOO_SOURCE_ERROR;
    /* Keep one byte until finish: EOF then accompanies actual final input,
     * without relying on a vendor-specific zero-length flush operation. */
    size_t ready=r->count;
    if (!r->ended && ready) --ready;
    if (!ready) return r->ended ? STREAMOO_SOURCE_END : STREAMOO_SOURCE_WAIT;
    if (ready>capacity) ready=capacity;
    size_t first=r->capacity-r->head; if (first>ready) first=ready;
    memcpy(out,r->data+r->head,first); memcpy(out+first,r->data,ready-first);
    r->head=(r->head+ready)%r->capacity; r->count-=ready;
    *bytes=ready; *last=r->ended && !r->count; return STREAMOO_SOURCE_DATA;
}
void streamoo_byte_ring_clear(streamoo_byte_ring_t *r)
{
    if (r) { r->head=0; r->count=0; r->ended=false; r->failed=false; }
}
