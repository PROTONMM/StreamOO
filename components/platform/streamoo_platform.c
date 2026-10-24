#include "streamoo_platform.h"

#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if !defined(CONFIG_SPIRAM_MODE_OCT) || !defined(CONFIG_SPIRAM_SPEED_80M) || !defined(CONFIG_ESPTOOLPY_FLASHSIZE_16MB)
#error "StreamOO N16R8 requires 16 MB flash and Octal PSRAM at 80 MHz"
#endif
#if CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ != 240
#error "StreamOO N16R8 profile requires CPU 240 MHz"
#endif

static const char *TAG = "platform";

static void log_heap(void)
{
    ESP_LOGI(TAG, "heap internal free=%zu min=%zu largest=%zu; PSRAM free=%zu min=%zu largest=%zu",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

static void health_task(void *context)
{
    (void)context;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        log_heap();
        ESP_LOGI(TAG, "health stack high-water=%u bytes",
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
    }
}

esp_err_t streamoo_platform_start(void)
{
    static bool started;
    if (started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_bytes = 0;
    esp_err_t error = esp_flash_get_size(NULL, &flash_bytes);
    if (error != ESP_OK) {
        return error;
    }
    const size_t psram_bytes = esp_psram_is_initialized() ? esp_psram_get_size() : 0;
    ESP_LOGI(TAG, "IDF %s; cores=%u; CPU profile=%d MHz; flash=%" PRIu32 "; PSRAM=%zu",
             esp_get_idf_version(), chip.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
             flash_bytes, psram_bytes);
    if (chip.model != CHIP_ESP32S3 || chip.cores != 2 ||
        flash_bytes != 16 * 1024 * 1024 || psram_bytes != 8 * 1024 * 1024) {
        ESP_LOGE(TAG, "Hardware does not match the N16R8 profile");
        return ESP_ERR_INVALID_STATE;
    }
    log_heap();
    if (xTaskCreate(health_task, "streamoo_health", 3072, NULL, 1, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    started = true;
    return ESP_OK;
}
