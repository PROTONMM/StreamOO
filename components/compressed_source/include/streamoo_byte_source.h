#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Task-only, single owner: externally serialize ALL producer/consumer calls.
 * No allocation, RTOS, atomics or I/O. Future network task needs a serialized
 * adapter; this queue is NOT an SPSC cross-task implementation. */
typedef enum { STREAMOO_SOURCE_DATA, STREAMOO_SOURCE_WAIT, STREAMOO_SOURCE_END,
    STREAMOO_SOURCE_ERROR } streamoo_source_result_t;
/* DATA: 1..capacity bytes, last=true only on final bytes. WAIT: no bytes and
 * no EOF; caller retries later. END: no bytes, EOF. ERROR: terminal failure.
 * Non-DATA calls must report bytes=0,last=false. Rewind is test-loop policy. */
typedef struct {
    void *context;
    streamoo_source_result_t (*read)(void *,uint8_t *,size_t,size_t *,bool *);
    bool (*rewind)(void *);
} streamoo_byte_source_t;
typedef struct {
    uint8_t *data;
    size_t capacity, head, count;
    bool ended, failed;
} streamoo_byte_ring_t;
bool streamoo_byte_ring_init(streamoo_byte_ring_t *,uint8_t *,size_t);
size_t streamoo_byte_ring_write(streamoo_byte_ring_t *,const uint8_t *,size_t);
void streamoo_byte_ring_finish(streamoo_byte_ring_t *,bool failed);
streamoo_source_result_t streamoo_byte_ring_read(void *,uint8_t *,size_t,size_t *,bool *);
/* Clears bytes and terminal status; caller must first stop all old producers. */
void streamoo_byte_ring_clear(streamoo_byte_ring_t *);
