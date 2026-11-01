#pragma once
#include "streamoo_byte_source.h"
/* Task-only serialized adapter, one consumer/rewind owner and one producer.
 * Storage is borrowed. Destroy ONLY after producer completion and no readers.
 * Copies occur under a mutex, never under an IRQ critical section. */
typedef struct streamoo_buffered_source streamoo_buffered_source_t;
typedef struct {
    uint32_t generation, backpressure, stale;
    size_t queued;
    bool stopped, ended;
} streamoo_buffered_source_stats_t;
streamoo_buffered_source_t *streamoo_buffered_source_create(uint8_t *,size_t);
void streamoo_buffered_source_destroy(streamoo_buffered_source_t *);
/* Nonblocking writes/finish. Rejected or partial bytes remain producer-owned.
 * Capture generation BEFORE reading/copying source data outside the mutex. */
size_t streamoo_buffered_source_write(streamoo_buffered_source_t *,uint32_t,const uint8_t *,size_t);
bool streamoo_buffered_source_finish(streamoo_buffered_source_t *,uint32_t,bool);
/* read: lock contention is WAIT. Others wait at most one tick-rounded 5ms. */
streamoo_source_result_t streamoo_buffered_source_read(void *,uint8_t *,size_t,size_t *,bool *);
bool streamoo_buffered_source_rewind(void *);
bool streamoo_buffered_source_stats(streamoo_buffered_source_t *,streamoo_buffered_source_stats_t *);
/* Session/lifetime diagnostic counters. Nonblocking loads, no mutex. Empty
 * includes the final retained byte of an unfinished live stream. */
bool streamoo_buffered_source_waits(const streamoo_buffered_source_t *,uint32_t *empty,uint32_t *busy);
bool streamoo_buffered_source_stop(streamoo_buffered_source_t *);
