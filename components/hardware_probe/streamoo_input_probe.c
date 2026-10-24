#include "streamoo_input_probe.h"

#include "sdkconfig.h"
#include "streamoo_input.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void probe_task(void *context)
{
    (void)context;
    const int pins[] = {CONFIG_STREAMOO_ENCODER_GPIO_A,
                        CONFIG_STREAMOO_ENCODER_GPIO_B,
                        CONFIG_STREAMOO_ENCODER_GPIO_BUTTON};
    int previous[3] = {gpio_get_level(pins[0]), gpio_get_level(pins[1]),
                       gpio_get_level(pins[2])};
    unsigned edges[3] = {0};
    streamoo_input_t decoder;
    streamoo_input_init(&decoder, previous[0], previous[1], !previous[2],
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
                        (uint64_t)esp_timer_get_time() / 1000);
    ESP_LOGI("input_probe", "TEST gestures: tick=%u ms; slow turns only; no UI/player commands",
             (unsigned)portTICK_PERIOD_MS);
    TickType_t last_log = xTaskGetTickCount();
    ESP_LOGI("input_probe", "Raw sampling only: edges are not detents; pulses may be missed");
    for (;;) {
        vTaskDelay(1);
        for (int i = 0; i < 3; ++i) {
            const int level = gpio_get_level(pins[i]);
            if (level != previous[i]) {
                ++edges[i];
                previous[i] = level;
            }
        }
        const streamoo_input_event_t event = streamoo_input_sample(
            &decoder, previous[0], previous[1], !previous[2],
            (uint64_t)esp_timer_get_time() / 1000);
        if (event.kind != STREAMOO_INPUT_NONE) {
            static const char *const names[] = {"NONE", "ROTATE", "SHORT", "LONG", "PRESSED_ROTATE"};
            ESP_LOGI("input_probe", "TEST %s delta=%d", names[event.kind], event.delta);
        }
        const TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - last_log) >= pdMS_TO_TICKS(1000)) {
            ESP_LOGI("input_probe", "A=%d B=%d BTN=%d sampled edges=%u/%u/%u",
                     previous[0], previous[1], previous[2], edges[0], edges[1], edges[2]);
            edges[0] = edges[1] = edges[2] = 0;
            last_log = now;
        }
    }
}

esp_err_t streamoo_input_probe_start(void)
{
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << CONFIG_STREAMOO_ENCODER_GPIO_A) |
                        (1ULL << CONFIG_STREAMOO_ENCODER_GPIO_B) |
                        (1ULL << CONFIG_STREAMOO_ENCODER_GPIO_BUTTON),
        .mode = GPIO_MODE_INPUT,
#if defined(CONFIG_STREAMOO_ENCODER_INTERNAL_PULLUP)
        .pull_up_en = GPIO_PULLUP_ENABLE,
#else
        .pull_up_en = GPIO_PULLUP_DISABLE,
#endif
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t error = gpio_config(&config);
    if (error != ESP_OK) {
        return error;
    }
    return xTaskCreate(probe_task, "input_probe", 3072, NULL, 1, NULL) == pdPASS
               ? ESP_OK : ESP_ERR_NO_MEM;
}
