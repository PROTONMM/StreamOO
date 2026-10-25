#include "audio_control_probe.h"
#include "audio_tone.h"
#include "streamoo_audio_gain.h"
#include "streamoo_audio_output.h"
#include "sdkconfig.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <inttypes.h>

static QueueHandle_t commands;
static portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
static streamoo_audio_control_state_t state;
static uint32_t target_gain;
typedef struct { uint32_t phase; streamoo_audio_gain_t gain; } render_t;
static DRAM_ATTR render_t render;

static void IRAM_ATTR controlled_fill(void *context, int32_t *lr, size_t frames)
{
    render_t *r=context;
    portENTER_CRITICAL_ISR(&lock);
    uint32_t target=target_gain;
    portEXIT_CRITICAL_ISR(&lock);
    streamoo_audio_gain_target(&r->gain,target);
    streamoo_audio_sine_fill(&r->phase,lr,frames);
    streamoo_audio_gain_process(&r->gain,lr,frames);
}
static void control_task(void *context)
{
    (void)context;
    streamoo_audio_output_t *output=NULL;
    const streamoo_audio_pins_t pins={CONFIG_STREAMOO_I2S_DOUT,CONFIG_STREAMOO_I2S_BCLK,CONFIG_STREAMOO_I2S_LRCLK};
    esp_err_t error=streamoo_audio_output_open(pins,controlled_fill,&render,&output);
    portENTER_CRITICAL(&lock);
    state.ready=error==ESP_OK; state.error=error;
    portEXIT_CRITICAL(&lock);
    ESP_LOGI("audio_control","TEST 1kHz stereo, source -30dBFS peak; initial STOP; gain 0..100, ramp 50ms; %s",esp_err_to_name(error));
    int64_t last_log=esp_timer_get_time();
    uint32_t last_buffers=0;
    while (error==ESP_OK) {
        streamoo_control_command_t command;
        if (xQueueReceive(commands,&command,pdMS_TO_TICKS(20) ? pdMS_TO_TICKS(20) : 1)==pdTRUE) {
            portENTER_CRITICAL(&lock);
            streamoo_audio_control_state_t next=state;
            portEXIT_CRITICAL(&lock);
            streamoo_audio_control_model_apply(&next.model,&command);
            uint32_t gain=0;
            streamoo_audio_control_model_gain(&next.model,&gain);
            portENTER_CRITICAL(&lock);
            state=next; target_gain=gain;
            portEXIT_CRITICAL(&lock);
        }
        const int64_t now=esp_timer_get_time();
        if (now-last_log>=5000000) {
            streamoo_audio_stats_t stats;
            streamoo_audio_output_stats(output,&stats);
            ESP_LOGI("audio_control","buffers=%" PRIu32 " max_fill=%" PRIu32 "us invalid=%" PRIu32,
                     stats.buffers,stats.max_fill_us,stats.invalid_buffers);
            if (stats.invalid_buffers || stats.max_fill_us>=5000) error=ESP_FAIL;
            if (stats.buffers==last_buffers) error=ESP_ERR_TIMEOUT;
            last_buffers=stats.buffers; last_log=now;
        }
    }
    portENTER_CRITICAL(&lock);
    state.ready=false; state.model.running=false; state.error=error;
    portEXIT_CRITICAL(&lock);
    if (output) {
        esp_err_t close_error=streamoo_audio_output_close(output);
        if (close_error!=ESP_OK) ESP_LOGE("audio_control","Close failed: %s",esp_err_to_name(close_error));
    }
    ESP_LOGE("audio_control","TEST stopped on error: %s",esp_err_to_name(error));
    vTaskDelete(NULL);
}
esp_err_t streamoo_audio_control_start(uint8_t volume)
{
    if (commands) return ESP_ERR_INVALID_STATE;
    if (volume>100) return ESP_ERR_INVALID_ARG;
    commands=xQueueCreate(16,sizeof(streamoo_control_command_t));
    if (!commands) return ESP_ERR_NO_MEM;
    state=(streamoo_audio_control_state_t){0};
    streamoo_audio_control_model_init(&state.model,volume);
    streamoo_audio_gain_init(&render.gain);
    if (xTaskCreate(control_task,"audio_control",4096,NULL,4,NULL)!=pdPASS) {
        vQueueDelete(commands); commands=NULL; return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
bool streamoo_audio_control_post(void *context, const streamoo_control_command_t *command)
{
    (void)context;
    if (!commands || !command || (command->kind!=STREAMOO_CONTROL_TOGGLE &&
        command->kind!=STREAMOO_CONTROL_VOLUME_DELTA)) return false;
    portENTER_CRITICAL(&lock);
    bool ready=state.ready;
    portEXIT_CRITICAL(&lock);
    return ready && xQueueSend(commands,command,0)==pdTRUE;
}
bool streamoo_audio_control_state(streamoo_audio_control_state_t *out)
{
    if (!commands || !out) return false;
    portENTER_CRITICAL(&lock);
    *out=state;
    portEXIT_CRITICAL(&lock);
    return true;
}
