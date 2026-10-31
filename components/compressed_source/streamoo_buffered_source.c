#include "streamoo_buffered_source.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdlib.h>
struct streamoo_buffered_source {
    streamoo_byte_ring_t ring;
    SemaphoreHandle_t mutex;
    uint32_t generation, backpressure, stale;
    bool stopped;
};
static TickType_t wait_ticks(void) { return pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1; }
streamoo_buffered_source_t *streamoo_buffered_source_create(uint8_t *storage,size_t capacity)
{
    streamoo_buffered_source_t *s=calloc(1,sizeof(*s));
    if (!s) return NULL;
    if (!streamoo_byte_ring_init(&s->ring,storage,capacity) || !(s->mutex=xSemaphoreCreateMutex())) {
        free(s); return NULL;
    }
    s->generation=1; return s;
}
void streamoo_buffered_source_destroy(streamoo_buffered_source_t *s)
{
    if (s) { vSemaphoreDelete(s->mutex); free(s); }
}
size_t streamoo_buffered_source_write(streamoo_buffered_source_t *s,uint32_t generation,
    const uint8_t *data,size_t bytes)
{
    if (!s || xSemaphoreTake(s->mutex,0)!=pdTRUE) return 0;
    size_t written=0;
    if (generation!=s->generation) ++s->stale;
    else if (!s->stopped) {
        written=streamoo_byte_ring_write(&s->ring,data,bytes);
        if (written<bytes) ++s->backpressure;
    }
    xSemaphoreGive(s->mutex); return written;
}
bool streamoo_buffered_source_finish(streamoo_buffered_source_t *s,uint32_t generation,bool failed)
{
    if (!s || xSemaphoreTake(s->mutex,0)!=pdTRUE) return false;
    bool ok=!s->stopped && generation==s->generation;
    if (generation!=s->generation) ++s->stale;
    if (ok) streamoo_byte_ring_finish(&s->ring,failed);
    xSemaphoreGive(s->mutex); return ok;
}
streamoo_source_result_t streamoo_buffered_source_read(void *context,uint8_t *data,size_t capacity,
    size_t *bytes,bool *last)
{
    if (bytes) *bytes=0;
    if (last) *last=false;
    streamoo_buffered_source_t *s=context;
    if (!s || !data || !capacity || !bytes || !last) return STREAMOO_SOURCE_ERROR;
    if (xSemaphoreTake(s->mutex,0)!=pdTRUE) return STREAMOO_SOURCE_WAIT;
    streamoo_source_result_t result=s->stopped ? STREAMOO_SOURCE_ERROR :
        streamoo_byte_ring_read(&s->ring,data,capacity,bytes,last);
    xSemaphoreGive(s->mutex); return result;
}
bool streamoo_buffered_source_rewind(void *context)
{
    streamoo_buffered_source_t *s=context;
    if (!s || xSemaphoreTake(s->mutex,wait_ticks())!=pdTRUE) return false;
    bool ok=!s->stopped && s->generation!=UINT32_MAX;
    if (ok) { ++s->generation; streamoo_byte_ring_clear(&s->ring); }
    xSemaphoreGive(s->mutex); return ok;
}
bool streamoo_buffered_source_stats(streamoo_buffered_source_t *s,streamoo_buffered_source_stats_t *out)
{
    if (!s || !out || xSemaphoreTake(s->mutex,wait_ticks())!=pdTRUE) return false;
    *out=(streamoo_buffered_source_stats_t){s->generation,s->backpressure,s->stale,
        s->ring.count,s->stopped,s->ring.ended};
    xSemaphoreGive(s->mutex); return true;
}
bool streamoo_buffered_source_stop(streamoo_buffered_source_t *s)
{
    if (!s || xSemaphoreTake(s->mutex,wait_ticks())!=pdTRUE) return false;
    s->stopped=true; xSemaphoreGive(s->mutex); return true;
}
