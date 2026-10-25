#include "audio_control_probe.h"
#include "audio_tone.h"
#include "mp3_fixture.h"
#include "streamoo_audio_gain.h"
#include "streamoo_pcm.h"
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
static streamoo_pcm_block_t pcm_storage[32], tap_storage[4];
static DRAM_ATTR streamoo_pcm_ring_t pcm_ring, tap_ring;
static DRAM_ATTR streamoo_pcm_pipeline_t render;
static streamoo_pcm_stats_t pcm_stats;
static streamoo_pcm_block_t producer_block;
static uint32_t producer_sequence, tap_blocks, tap_peak, tap_gaps;
#ifndef CONFIG_STREAMOO_MP3_FIXTURE_TEST
static uint32_t producer_phase;
#endif
#ifndef CONFIG_STREAMOO_PCM_TAP_DELAY_MS
#define CONFIG_STREAMOO_PCM_TAP_DELAY_MS 0
#endif
_Static_assert(STREAMOO_PCM_FRAMES==STREAMOO_AUDIO_DMA_FRAMES,"PCM and DMA blocks must match");
static bool tap_alive;

static bool produce_pcm(void)
{
    for (unsigned i=0;i<32 && streamoo_pcm_count(&pcm_ring)<32;++i) {
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
        if (!streamoo_mp3_fixture_read(producer_block.samples,STREAMOO_PCM_FRAMES)) return false;
#else
        if (!streamoo_audio_sine_fill(&producer_phase,producer_block.samples,STREAMOO_PCM_FRAMES)) return false;
#endif
        if (!streamoo_pcm_push(&pcm_ring,producer_block.samples,producer_sequence)) return false;
        ++producer_sequence;
    }
    return true;
}
static void tap_task(void *context)
{
    (void)context;
    streamoo_pcm_block_t block;
    bool seen=false; uint32_t previous=0, sequence=0;
    for (;;) {
        portENTER_CRITICAL(&lock);
        bool alive=tap_alive;
        portEXIT_CRITICAL(&lock);
        if (!alive) break;
        for (unsigned n=0;n<4 && streamoo_pcm_pop(&tap_ring,block.samples,&sequence);++n) {
            uint32_t peak=0;
            for (unsigned i=0;i<STREAMOO_PCM_WORDS;++i) {
                uint32_t value=block.samples[i]<0 ? 0u-(uint32_t)block.samples[i] : (uint32_t)block.samples[i];
                if (value>peak) peak=value;
            }
            portENTER_CRITICAL(&lock);
            ++tap_blocks; tap_peak=peak;
            if (seen) tap_gaps+=sequence-previous-1;
            seen=true; previous=sequence;
            portEXIT_CRITICAL(&lock);
        }
        vTaskDelay(pdMS_TO_TICKS(CONFIG_STREAMOO_PCM_TAP_DELAY_MS) ? pdMS_TO_TICKS(CONFIG_STREAMOO_PCM_TAP_DELAY_MS) : 1);
    }
    vTaskDelete(NULL);
}

static void IRAM_ATTR controlled_fill(void *context, int32_t *lr, size_t frames)
{
    streamoo_pcm_pipeline_t *r=context;
    portENTER_CRITICAL_ISR(&lock);
    uint32_t target=target_gain;
    portEXIT_CRITICAL_ISR(&lock);
    streamoo_pcm_fill(r,lr,frames,target);
    portENTER_CRITICAL_ISR(&lock);
    pcm_stats=r->stats;
    portEXIT_CRITICAL_ISR(&lock);
}
static void control_task(void *context)
{
    (void)context;
    streamoo_audio_output_t *output=NULL;
    const streamoo_audio_pins_t pins={CONFIG_STREAMOO_I2S_DOUT,CONFIG_STREAMOO_I2S_BCLK,CONFIG_STREAMOO_I2S_LRCLK};
    esp_err_t error=ESP_OK;
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
    error=streamoo_mp3_fixture_prepare();
#endif
    if (error==ESP_OK && !produce_pcm()) error=ESP_FAIL;
    if (error==ESP_OK) error=streamoo_audio_output_open(pins,controlled_fill,&render,&output);
    if (error==ESP_OK && xTaskCreate(tap_task,"pcm_tap",4096,NULL,1,NULL)!=pdPASS) error=ESP_ERR_NO_MEM;
    portENTER_CRITICAL(&lock);
    state.ready=error==ESP_OK; state.error=error;
    portEXIT_CRITICAL(&lock);
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
    const char *source="embedded MP3 L/R clip";
#else
    const char *source="1kHz stereo sine";
#endif
    ESP_LOGI("audio_control","TEST source=%s; initial STOP; gain 0..100, ramp 50ms; %s",source,esp_err_to_name(error));
#ifdef CONFIG_STREAMOO_PCM_STALL_TEST
    const char *stall_profile="ON";
#else
    const char *stall_profile="OFF";
#endif
    ESP_LOGI("audio_control","PCM test profile: producer_stall=%s tap_delay_ms=%d",
             stall_profile,CONFIG_STREAMOO_PCM_TAP_DELAY_MS);
    int64_t last_log=esp_timer_get_time();
    uint32_t last_buffers=0;
#ifdef CONFIG_STREAMOO_PCM_STALL_TEST
    int64_t first_play=-1, stall_start=-1;
    bool stall_done=false;
#endif
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
        bool stall=false;
#ifdef CONFIG_STREAMOO_PCM_STALL_TEST
        if (state.model.running && first_play<0) first_play=now;
        if (!stall_done && first_play>=0 && now-first_play>=3000000) {
            if (stall_start<0) {
                stall_start=now;
                ESP_LOGW("audio_control","TEST PCM producer stall for 400ms");
            }
            stall=now-stall_start<400000;
            if (!stall) { stall_done=true; ESP_LOGI("audio_control","TEST PCM producer resumed"); }
        }
#endif
        if (!stall && !produce_pcm()) error=ESP_FAIL;
        if (now-last_log>=5000000) {
            streamoo_audio_stats_t stats;
            streamoo_audio_output_stats(output,&stats);
            ESP_LOGI("audio_control","buffers=%" PRIu32 " max_fill=%" PRIu32 "us invalid=%" PRIu32,
                     stats.buffers,stats.max_fill_us,stats.invalid_buffers);
            portENTER_CRITICAL(&lock);
            streamoo_pcm_stats_t pcm=pcm_stats;
            uint32_t analyzed=tap_blocks, peak=tap_peak, gaps=tap_gaps;
            portEXIT_CRITICAL(&lock);
            ESP_LOGI("audio_control","PCM queued=%" PRIu32 " mode=%u low=%" PRIu32 " hard=%" PRIu32 " starts=%" PRIu32 " tap_drop=%" PRIu32 " tap_blocks=%" PRIu32 " tap_gaps=%" PRIu32 " tap_peak16=%" PRIu32,
                     pcm.queued,(unsigned)pcm.mode,pcm.underflows,pcm.hard_underflows,pcm.restarts,pcm.tap_drops,analyzed,gaps,peak>>16);
            if (stats.invalid_buffers || stats.max_fill_us>=5000) error=ESP_FAIL;
            if (stats.buffers==last_buffers) error=ESP_ERR_TIMEOUT;
            last_buffers=stats.buffers; last_log=now;
        }
    }
    portENTER_CRITICAL(&lock);
    state.ready=false; state.model.running=false; state.error=error; tap_alive=false;
    portEXIT_CRITICAL(&lock);
    if (output) {
        esp_err_t close_error=streamoo_audio_output_close(output);
        if (close_error!=ESP_OK) ESP_LOGE("audio_control","Close failed: %s",esp_err_to_name(close_error));
    }
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
    streamoo_mp3_fixture_release();
#endif
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
    streamoo_pcm_ring_init(&pcm_ring,pcm_storage,32);
    streamoo_pcm_ring_init(&tap_ring,tap_storage,4);
    streamoo_pcm_pipeline_init(&render,&pcm_ring,&tap_ring);
    tap_alive=true;
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
    const uint32_t stack_bytes=24576;
#else
    const uint32_t stack_bytes=4096;
#endif
    if (xTaskCreate(control_task,"audio_control",stack_bytes,NULL,4,NULL)!=pdPASS) {
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
    out->buffering=pcm_stats.mode!=STREAMOO_PCM_ACTIVE;
    portEXIT_CRITICAL(&lock);
    return true;
}
