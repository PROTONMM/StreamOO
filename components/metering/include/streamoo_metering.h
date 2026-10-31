#pragma once
#include "esp_err.h"
#include "streamoo_levels.h"
#include "streamoo_pcm.h"

typedef struct streamoo_metering streamoo_metering_t;
typedef struct {
    streamoo_levels_result_t levels;
    uint64_t generation;
    int64_t completed_us;
    uint32_t max_work_us, stack_hwm;
    bool spectrum_valid, spectrum_available;
    uint16_t bands[24];
    uint32_t spectrum_request, fft_windows, fft_skipped, fft_gaps, max_fft_us;
    int64_t spectrum_completed_us;
} streamoo_metering_state_t;
/* Task-only lifecycle owner is the player. Independent low-priority consumer
 * of the existing before-gain SPSC tap. Internal RAM, fixed four-block queue.
 * Open before I2S; attach tap while pipeline is quiescent. No backpressure.
 * Allocation/task failure disables metering, not playback. */
esp_err_t streamoo_metering_open(uint64_t generation, streamoo_metering_t **);
streamoo_pcm_ring_t *streamoo_metering_tap(streamoo_metering_t *);
/* Copies a coherent snapshot, invalidates results older than200ms. */
bool streamoo_metering_state(streamoo_metering_t *, streamoo_metering_state_t *);
/* ONLY after I2S close/producer quiescence. Async stop; false until consumer
 * ACK, then frees all context. Caller must clear its pointer on true. */
bool streamoo_metering_dispose(streamoo_metering_t *);

/* Single lifecycle-owner task. Changes invalidate previous spectrum at once. */
void streamoo_metering_set_spectrum(streamoo_metering_t *, bool enabled);
