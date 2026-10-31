#include "streamoo_metering.h"
#include "streamoo_ballistics.h"
#include "streamoo_spectrum_stream.h"
#include "streamoo_spectrum_envelope.h"
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdatomic.h>

struct streamoo_metering {
    streamoo_pcm_ring_t tap;
    streamoo_pcm_block_t storage[4], block;
    streamoo_levels_t analyzer;
    streamoo_metering_state_t state;
    atomic_bool stop, done;
    atomic_uint spectrum_request;
};
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static void analyze(void *context) {
    streamoo_metering_t *m = context;
    streamoo_metering_state_t current = {.generation = m->state.generation};
    streamoo_ballistics_t ballistics;
    streamoo_ballistics_init(&ballistics, esp_timer_get_time());
    streamoo_spectrum_stream_t *spectrum = NULL;
    uint32_t request_seen = UINT32_MAX;
    int64_t fft_at = 0;
    streamoo_spectrum_envelope_t spectrum_envelope;
    streamoo_spectrum_envelope_init(&spectrum_envelope,esp_timer_get_time());
    for (;;) {
        if (atomic_load_explicit(&m->stop, memory_order_acquire)) break;
        int64_t start = esp_timer_get_time();
        uint32_t request = atomic_load_explicit(&m->spectrum_request, memory_order_acquire);
        if(request != request_seen) {
            request_seen = request;
            current.spectrum_valid = false;
            memset(current.bands,0,sizeof current.bands);
            current.spectrum_request = request;
            fft_at = 0;
            streamoo_spectrum_envelope_init(&spectrum_envelope,start);
            if((request&1u) && !spectrum) {
                spectrum = heap_caps_calloc(1,sizeof *spectrum,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
                if(spectrum)streamoo_spectrum_stream_init(spectrum);
            }
            if(spectrum)streamoo_spectrum_stream_reset(spectrum);
            current.spectrum_available = spectrum != NULL;
        }
        for (unsigned n = 0; n < 4 &&
             streamoo_pcm_pop(&m->tap, m->block.samples, &m->block.sequence); ++n) {
            if((request&1u) && spectrum) {
                int64_t fft_start = esp_timer_get_time();
                if(streamoo_spectrum_stream_feed(spectrum,m->block.samples,
                        STREAMOO_PCM_FRAMES,m->block.sequence,fft_start>=fft_at)) {
                    current.spectrum_completed_us = esp_timer_get_time();
                    uint32_t elapsed_fft = (uint32_t)(current.spectrum_completed_us-fft_start);
                    if(elapsed_fft>current.max_fft_us)current.max_fft_us=elapsed_fft;
                    fft_at=current.spectrum_completed_us+100000;
                    uint16_t target[24];
                    for(unsigned b=0;b<24;++b)
                        target[b]=streamoo_spectrum_display_level(spectrum->fft.bands[b]);
                    streamoo_spectrum_envelope_feed(&spectrum_envelope,target,current.spectrum_completed_us);
                    memcpy(current.bands,spectrum_envelope.bands,sizeof current.bands);
                    current.spectrum_valid=true;
                }
                current.fft_windows=spectrum->windows;
                current.fft_skipped=spectrum->skipped;
                current.fft_gaps=spectrum->gaps;
            }
            uint64_t previous = m->analyzer.result.windows;
            streamoo_levels_process(&m->analyzer, m->block.samples,
                                     STREAMOO_PCM_FRAMES, m->block.sequence);
            current.levels.windows = m->analyzer.result.windows;
            current.levels.discontinuities = m->analyzer.result.discontinuities;
            if (m->analyzer.result.valid && m->analyzer.result.windows != previous) {
                streamoo_ballistics_feed(&ballistics, &m->analyzer.result);
                current.completed_us = esp_timer_get_time();
                current.levels.valid = true;
            }
        }
        streamoo_ballistics_update(&ballistics, esp_timer_get_time(),
                                   current.levels.rms, current.levels.peak);
        uint32_t elapsed = (uint32_t)(esp_timer_get_time() - start);
        if (elapsed > current.max_work_us) current.max_work_us = elapsed;
        current.stack_hwm = uxTaskGetStackHighWaterMark(NULL);
        portENTER_CRITICAL(&lock);
        m->state = current;
        portEXIT_CRITICAL(&lock);
        vTaskDelay(pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1);
    }
    heap_caps_free(spectrum);
    /* No context access after release: lifecycle owner may now free it. */
    atomic_store_explicit(&m->done, true, memory_order_release);
    vTaskDelete(NULL);
}
esp_err_t streamoo_metering_open(uint64_t generation, streamoo_metering_t **out) {
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = NULL;
    streamoo_metering_t *m = heap_caps_calloc(1, sizeof *m,
                                            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!m) return ESP_ERR_NO_MEM;
    atomic_init(&m->stop, false);
    atomic_init(&m->done, false);
    atomic_init(&m->spectrum_request,0);
    m->state.generation = generation;
    streamoo_pcm_ring_init(&m->tap, m->storage, 4);
    streamoo_levels_init(&m->analyzer, STREAMOO_LEVELS_WINDOW_DEFAULT);
    if (xTaskCreate(analyze, "audio_levels", 4096, m, 1, NULL) != pdPASS) {
        heap_caps_free(m);
        return ESP_ERR_NO_MEM;
    }
    *out = m;
    return ESP_OK;
}
streamoo_pcm_ring_t *streamoo_metering_tap(streamoo_metering_t *m) {
    return m ? &m->tap : NULL;
}
bool streamoo_metering_state(streamoo_metering_t *m, streamoo_metering_state_t *out) {
    if (!m || !out) return false;
    portENTER_CRITICAL(&lock);
    *out = m->state;
    portEXIT_CRITICAL(&lock);
    int64_t now = esp_timer_get_time();
    if (now < out->completed_us || now - out->completed_us > 200000) {
        out->levels.valid = false;
        out->levels.rms[0] = out->levels.rms[1] = 0;
        out->levels.peak[0] = out->levels.peak[1] = 0;
    }
    uint32_t request=atomic_load_explicit(&m->spectrum_request,memory_order_acquire);
    if(!(request&1u) || request!=out->spectrum_request ||
       now<out->spectrum_completed_us || now-out->spectrum_completed_us>200000) {
        out->spectrum_valid=false;
        memset(out->bands,0,sizeof out->bands);
    }
    return true;
}
void streamoo_metering_set_spectrum(streamoo_metering_t *m,bool enabled) {
    if(!m)return;
    uint32_t request=atomic_load_explicit(&m->spectrum_request,memory_order_relaxed);
    if((bool)(request&1u)!=enabled)
        atomic_store_explicit(&m->spectrum_request,request+1,memory_order_release);
}
bool streamoo_metering_dispose(streamoo_metering_t *m) {
    if (!m) return true;
    atomic_store_explicit(&m->stop, true, memory_order_release);
    if (!atomic_load_explicit(&m->done, memory_order_acquire)) return false;
    heap_caps_free(m);
    return true;
}
