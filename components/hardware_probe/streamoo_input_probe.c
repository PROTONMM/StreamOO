#include "streamoo_input_probe.h"
#include "sdkconfig.h"
#include "streamoo_input.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <inttypes.h>

/* Diagnostic transport only. ISR never logs, allocates or touches UI. */
static const int pins[] = {CONFIG_STREAMOO_ENCODER_GPIO_A,
                          CONFIG_STREAMOO_ENCODER_GPIO_B,
                          CONFIG_STREAMOO_ENCODER_GPIO_BUTTON};
typedef struct { uint64_t ms; uint8_t levels; } edge_t;
static QueueHandle_t queue;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static uint32_t edges[3], lost;
static streamoo_input_t decoder;
static edge_t current;

static edge_t read_levels(void)
{
    edge_t e = {.ms = (uint64_t)esp_timer_get_time() / 1000};
    for (unsigned i = 0; i < 3; ++i)
        e.levels |= gpio_get_level(pins[i]) << i;
    return e;
}

static void initialize_decoder(edge_t e)
{
    streamoo_input_init(&decoder, e.levels & 1, e.levels & 2, !(e.levels & 4),
#ifdef CONFIG_STREAMOO_ENCODER_HALF_QUADRATURE
                        true,
#else
                        false,
#endif
#ifdef CONFIG_STREAMOO_ENCODER_REVERSED
                        true,
#else
                        false,
#endif
                        e.ms);
}

static void edge_isr(void *arg)
{
    unsigned pin_index = (unsigned)(uintptr_t)arg;
    const edge_t e = read_levels();
    BaseType_t wake = pdFALSE;
    portENTER_CRITICAL_ISR(&lock);
    ++edges[pin_index];
    if (xQueueSendFromISR(queue, &e, &wake) != pdTRUE) ++lost;
    portEXIT_CRITICAL_ISR(&lock);
    if (wake) portYIELD_FROM_ISR();
}

static void decode(edge_t e)
{
    /* Timeout and a concurrently arriving ISR can share a millisecond. Keep
     * the decoder monotonic even when that edge was captured before timeout. */
    if (e.ms < current.ms) e.ms = current.ms;
    current = e;
    streamoo_input_event_t event = streamoo_input_sample(
        &decoder, e.levels & 1, e.levels & 2, !(e.levels & 4), e.ms);
    if (event.kind != STREAMOO_INPUT_NONE) {
        static const char *const names[] = {"NONE", "ROTATE", "SHORT", "LONG", "PRESSED_ROTATE"};
        ESP_LOGI("input_probe", "TEST %s delta=%d", names[event.kind], event.delta);
    }
}

static void probe_task(void *context)
{
    (void)context;
    ESP_LOGI("input_probe", "TEST IRQ capture: A=GPIO%d B=GPIO%d BTN=GPIO%d; half=%d reverse=%d",
             pins[0], pins[1], pins[2], decoder.half, decoder.reversed);
    ESP_LOGI("input_probe", "Expected: both A/B IRQ counts change; opposite rotation has opposite delta");
    uint64_t last_log = current.ms;
    for (;;) {
        edge_t e;
        bool received = xQueueReceive(queue, &e, 1) == pdTRUE;
        portENTER_CRITICAL(&lock);
        uint32_t dropped = lost;
        lost = 0;
        portEXIT_CRITICAL(&lock);
        if (dropped) {
            /* Lost ordering cannot be decoded safely. Stop capture, discard
             * pending samples, and re-arm from actual levels; no inferred click. */
            for (unsigned i = 0; i < 3; ++i) gpio_intr_disable(pins[i]);
            xQueueReset(queue);
            current = read_levels();
            initialize_decoder(current);
            decoder.armed = false;
            for (unsigned i = 0; i < 3; ++i) gpio_intr_enable(pins[i]);
            ESP_LOGW("input_probe", "IRQ queue overflow: dropped=%" PRIu32 "; decoder reset, repeat test", dropped);
        } else if (received) {
            decode(e);
        } else {
            /* Advance debounce using the last captured levels, not a polled
             * future level that could overtake an already queued edge. */
            e = current;
            e.ms = (uint64_t)esp_timer_get_time() / 1000;
            decode(e);
        }
        uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
        if (now - last_log >= 1000) {
            uint32_t counts[3];
            portENTER_CRITICAL(&lock);
            for (unsigned i = 0; i < 3; ++i) { counts[i] = edges[i]; edges[i] = 0; }
            portEXIT_CRITICAL(&lock);
            edge_t levels = read_levels();
            ESP_LOGI("input_probe", "A=%d B=%d BTN=%d IRQ edges=%" PRIu32 "/%" PRIu32 "/%" PRIu32,
                     !!(levels.levels & 1), !!(levels.levels & 2), !!(levels.levels & 4),
                     counts[0], counts[1], counts[2]);
            if (counts[0] >= 4 && counts[1] == 0)
                ESP_LOGW("input_probe", "A changes, B has no IRQ: verify B signal on GPIO%d; direction unavailable", pins[1]);
            if (counts[1] >= 4 && counts[0] == 0)
                ESP_LOGW("input_probe", "B changes, A has no IRQ: verify A signal on GPIO%d; direction unavailable", pins[0]);
            last_log = now;
        }
    }
}

esp_err_t streamoo_input_probe_start(void)
{
    if (queue) return ESP_ERR_INVALID_STATE;
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << CONFIG_STREAMOO_ENCODER_GPIO_A) |
                        (1ULL << CONFIG_STREAMOO_ENCODER_GPIO_B) |
                        (1ULL << CONFIG_STREAMOO_ENCODER_GPIO_BUTTON),
        .mode = GPIO_MODE_INPUT,
#ifdef CONFIG_STREAMOO_ENCODER_INTERNAL_PULLUP
        .pull_up_en = GPIO_PULLUP_ENABLE,
#else
        .pull_up_en = GPIO_PULLUP_DISABLE,
#endif
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t error = gpio_config(&config);
    if (error != ESP_OK) return error;
    queue = xQueueCreate(128, sizeof(edge_t));
    if (!queue) return ESP_ERR_NO_MEM;
    /* Own the service; do not silently inherit another component's flags. */
    error = gpio_install_isr_service(0);
    if (error != ESP_OK) { vQueueDelete(queue); queue = NULL; return error; }
    current = read_levels();
    initialize_decoder(current);
    unsigned installed = 0;
    for (unsigned i = 0; i < 3; ++i) {
        error = gpio_isr_handler_add(pins[i], edge_isr, (void *)(uintptr_t)i);
        if (error != ESP_OK) goto fail;
        ++installed;
        error = gpio_set_intr_type(pins[i], GPIO_INTR_ANYEDGE);
        if (error != ESP_OK) goto fail;
        error = gpio_intr_enable(pins[i]);
        if (error != ESP_OK) goto fail;
    }
    if (xTaskCreate(probe_task, "input_probe", 4096, NULL, 3, NULL) == pdPASS)
        return ESP_OK;
    error = ESP_ERR_NO_MEM;
fail:
    for (unsigned i = 0; i < installed; ++i) {
        gpio_intr_disable(pins[i]);
        gpio_isr_handler_remove(pins[i]);
    }
    gpio_uninstall_isr_service();
    vQueueDelete(queue);
    queue = NULL;
    return error;
}
