#include "streamoo_metering.h"
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
};
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static void analyze(void *context) {
    streamoo_metering_t *m = context;
    streamoo_metering_state_t current = {.generation = m->state.generation};
    for (;;) {
        if (atomic_load_explicit(&m->stop, memory_order_acquire)) break;
        int64_t start = esp_timer_get_time();
        for (unsigned n = 0; n < 4 &&
             streamoo_pcm_pop(&m->tap, m->block.samples, &m->block.sequence); ++n) {
            uint64_t previous = m->analyzer.result.windows;
            streamoo_levels_process(&m->analyzer, m->block.samples,
                                     STREAMOO_PCM_FRAMES, m->block.sequence);
            current.levels = m->analyzer.result;
            if (current.levels.windows != previous)
                current.completed_us = esp_timer_get_time();
        }
        uint32_t elapsed = (uint32_t)(esp_timer_get_time() - start);
        if (elapsed > current.max_work_us) current.max_work_us = elapsed;
        current.stack_hwm = uxTaskGetStackHighWaterMark(NULL);
        portENTER_CRITICAL(&lock);
        m->state = current;
        portEXIT_CRITICAL(&lock);
        vTaskDelay(pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1);
    }
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
    return true;
}
bool streamoo_metering_dispose(streamoo_metering_t *m) {
    if (!m) return true;
    atomic_store_explicit(&m->stop, true, memory_order_release);
    if (!atomic_load_explicit(&m->done, memory_order_acquire)) return false;
    heap_caps_free(m);
    return true;
}
