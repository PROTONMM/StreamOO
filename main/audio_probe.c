#include "audio_probe.h"
#include "audio_tone.h"
#include "streamoo_audio_output.h"
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>

static DRAM_ATTR streamoo_audio_tone_t tone;
static bool started;
static void IRAM_ATTR tone_dma_fill(void *context, int32_t *lr, size_t frames)
{
    (void)streamoo_audio_tone_fill(context,lr,frames);
}
static void audio_task(void *context)
{
    (void)context;
    streamoo_audio_output_t *output=NULL;
    const streamoo_audio_pins_t pins={CONFIG_STREAMOO_I2S_DOUT,CONFIG_STREAMOO_I2S_BCLK,CONFIG_STREAMOO_I2S_LRCLK};
    ESP_LOGI("audio_probe","TEST: 48kHz/32-bit stereo, DOUT=%d BCLK=%d WS=%d; fixed -30dBFS, independent of UI VOL",pins.dout,pins.bclk,pins.ws);
    ESP_LOGI("audio_probe","Sequence: zero 1s, L1k 2s, zero 1s, R2k 2s, zero 1s, both1k 2s, zero 1s");
    esp_err_t error=streamoo_audio_output_open(pins,tone_dma_fill,&tone,&output);
    const int64_t start=esp_timer_get_time();
    streamoo_audio_stats_t stats={0};
    const uint32_t target=STREAMOO_TONE_FRAMES/STREAMOO_AUDIO_DMA_FRAMES+STREAMOO_AUDIO_DMA_BUFFERS;
    while (error==ESP_OK && stats.buffers<target) {
        vTaskDelay(pdMS_TO_TICKS(20) ? pdMS_TO_TICKS(20) : 1);
        streamoo_audio_output_stats(output,&stats);
        if (stats.invalid_buffers || stats.max_fill_us>=5000) error=ESP_FAIL;
        if (esp_timer_get_time()-start>15000000) error=ESP_ERR_TIMEOUT;
    }
    if (output) {
        const esp_err_t close_error=streamoo_audio_output_close(output);
        if (error==ESP_OK) error=close_error;
    }
    ESP_LOGI("audio_probe","TEST ended: %s; buffers=%" PRIu32 " max_fill=%" PRIu32 "us invalid=%" PRIu32,
             esp_err_to_name(error),stats.buffers,stats.max_fill_us,stats.invalid_buffers);
    ESP_LOGI("audio_probe","Channel sound/order and clock measurements remain HARDWARE_TEST_PENDING");
    vTaskDelete(NULL);
}
esp_err_t streamoo_audio_probe_start(void)
{
    if (started) return ESP_ERR_INVALID_STATE;
    if (xTaskCreate(audio_task,"audio_probe",4096,NULL,4,NULL)!=pdPASS) return ESP_ERR_NO_MEM;
    started=true;
    return ESP_OK;
}
