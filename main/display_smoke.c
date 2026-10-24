#include "display_smoke.h"
#include "encoder_demo.h"
#include "streamoo_preferences.h"
#include "streamoo_input_probe.h"
#include "canvas_smoke.h"
#include "text_smoke.h"
#include "home_probe.h"
#include "ui_list_probe.h"

#include <inttypes.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "streamoo_ssd1322.h"

static void display_task(void *context)
{
    (void)context;
    const streamoo_ssd1322_config_t config = {
        .spi_host = CONFIG_STREAMOO_LCD_SPI_HOST,
        .mosi = CONFIG_STREAMOO_LCD_MOSI, .sclk = CONFIG_STREAMOO_LCD_SCLK,
        .cs = CONFIG_STREAMOO_LCD_CS, .dc = CONFIG_STREAMOO_LCD_DC,
        .reset = CONFIG_STREAMOO_LCD_RST,
        .clock_hz = CONFIG_STREAMOO_LCD_SPI_CLOCK_MHZ * 1000000,
        .panel = {.column_offset = CONFIG_STREAMOO_LCD_COLUMN_OFFSET,
                  .remap = CONFIG_STREAMOO_LCD_REMAP,
                  .contrast = 0x7f, .master_current = 3},
    };
    streamoo_ssd1322_t *display = NULL;
    uint8_t *frame = heap_caps_calloc(1, SSD1322_FRAME_BYTES,
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    esp_err_t error = frame ? streamoo_ssd1322_open(&config, &display) : ESP_ERR_NO_MEM;
    if (error == ESP_OK) error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
    if (error == ESP_OK) error = streamoo_ssd1322_enable(display, true);
#ifdef CONFIG_STREAMOO_ENCODER_UI_TEST
    if (error == ESP_OK) {
        streamoo_encoder_demo_t demo;
        if (!streamoo_encoder_demo_init(&demo)) error = ESP_ERR_INVALID_STATE;
        streamoo_preferences_value_t restored = {50,0};
        const bool preferences_ready = streamoo_preferences_initial(&restored);
        demo.volume = restored.volume;
        if (!streamoo_ui_set_visualization(&demo.ui, restored.visualization)) error = ESP_ERR_INVALID_STATE;
        streamoo_input_event_t event;
        /* Discard input captured before this UI session was ready. */
        while (streamoo_input_probe_receive(&event)) {}
        TickType_t wake = xTaskGetTickCount();
        ESP_LOGI("display_smoke", "Interactive encoder TEST ready: no audio/network; volume/widget preferences enabled if available");
        while (error == ESP_OK) {
            uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
            for (unsigned n=0; n<32 && streamoo_input_probe_receive(&event); ++n) {
#ifdef CONFIG_STREAMOO_ENCODER_MODE_B
                const streamoo_control_mode_t mode = STREAMOO_CONTROL_MODE_B;
#else
                const streamoo_control_mode_t mode = STREAMOO_CONTROL_MODE_A;
#endif
                if (!streamoo_encoder_demo_input(&demo,event,mode,now))
                    ESP_LOGW("display_smoke", "TEST gesture rejected; state preserved");
                const streamoo_preferences_value_t preferences = {(uint8_t)demo.volume, (uint8_t)demo.ui.visualization};
                if (preferences_ready && !streamoo_preferences_submit(preferences))
                    ESP_LOGW("display_smoke", "Preferences update not accepted");
            }
            if (!streamoo_encoder_demo_draw(&demo,frame,SSD1322_FRAME_BYTES,now)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            error = streamoo_ssd1322_present(display,frame,SSD1322_FRAME_BYTES);
            vTaskDelayUntil(&wake,pdMS_TO_TICKS(40) ? pdMS_TO_TICKS(40) : 1);
        }
    }
#else
    if (error == ESP_OK) {
        ESP_LOGW("display_smoke", "Experimental panel profile; verify image on OLED. RESET GPIO=%d",
                 CONFIG_STREAMOO_LCD_RST);
        /* Finite test avoids leaving a bright diagnostic image indefinitely. */
        for (int step = 0; step < 6 && error == ESP_OK; ++step) {
            if (step == 0) memset(frame, 0x00, SSD1322_FRAME_BYTES);
            if (step == 1) memset(frame, 0xff, SSD1322_FRAME_BYTES);
            if (step == 2) ssd1322_test_pattern(frame, SSD1322_FRAME_BYTES);
            if (step == 3) {
                for (size_t i = 0; i < SSD1322_FRAME_BYTES; ++i) frame[i] ^= 0xff;
            }
            if (step == 4) {
                const int64_t render_start = esp_timer_get_time();
                if (!streamoo_canvas_smoke(frame, SSD1322_FRAME_BYTES)) {
                    error = ESP_ERR_INVALID_ARG;
                    break;
                }
                ESP_LOGI("display_smoke", "canvas scene raster=%" PRId64 " us",
                         esp_timer_get_time() - render_start);
            }
            if (step == 5 && !streamoo_text_smoke(frame, SSD1322_FRAME_BYTES)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            const int64_t start = esp_timer_get_time();
            error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
            ESP_LOGI("display_smoke", "step=%d frame transfer=%" PRId64 " us, result=%s",
                     step, esp_timer_get_time() - start, esp_err_to_name(error));
            if (error == ESP_OK) vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
    if (error == ESP_OK) {
        streamoo_home_probe_t home;
        if (!streamoo_home_probe_init(&home)) error = ESP_ERR_INVALID_STATE;
        const int64_t animation_start = esp_timer_get_time();
        TickType_t wake = xTaskGetTickCount();
        unsigned frames = 0;
        while (error == ESP_OK && esp_timer_get_time() - animation_start < 20000000) {
            const uint64_t elapsed_ms = (esp_timer_get_time() - animation_start) / 1000;
            if (!streamoo_home_probe_draw(&home, frame, SSD1322_FRAME_BYTES, elapsed_ms)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
            ++frames;
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(40) ? pdMS_TO_TICKS(40) : 1);
        }
        ESP_LOGI("display_smoke", "marquee frames=%u elapsed=%" PRId64 " us",
                 frames, esp_timer_get_time() - animation_start);
    }
    if (error == ESP_OK) {
        streamoo_ui_list_probe_t probe;
        if (!streamoo_ui_list_probe_init(&probe)) error = ESP_ERR_INVALID_STATE;
        const int64_t list_start = esp_timer_get_time();
        TickType_t wake = xTaskGetTickCount();
        while (error == ESP_OK && esp_timer_get_time() - list_start < 8000000) {
            const uint64_t elapsed = (esp_timer_get_time() - list_start) / 1000;
            if (!streamoo_ui_list_probe_draw(&probe, frame, SSD1322_FRAME_BYTES, elapsed)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(40) ? pdMS_TO_TICKS(40) : 1);
        }
        if (error == ESP_OK)
            ESP_LOGI("display_smoke", "list diagnostic requested=%" PRIu64 " snapshot=%" PRIu64,
                     probe.requested_id, probe.ui.playing_id);
    }
#endif
    if (error != ESP_OK) ESP_LOGE("display_smoke", "Failed: %s", esp_err_to_name(error));
    if (display) {
        const esp_err_t close_error = streamoo_ssd1322_close(display);
        if (close_error != ESP_OK) ESP_LOGE("display_smoke", "Close failed: %s", esp_err_to_name(close_error));
    }
    heap_caps_free(frame);
    ESP_LOGI("display_smoke", "Test ended; visual acceptance remains HARDWARE_TEST_PENDING");
    vTaskDelete(NULL);
}

esp_err_t streamoo_display_smoke_start(void)
{
    return xTaskCreate(display_task, "display_smoke", 4096, NULL, 2, NULL) == pdPASS
               ? ESP_OK : ESP_ERR_NO_MEM;
}
