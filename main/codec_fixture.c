#include "codec_fixture.h"
#include "sdkconfig.h"
#ifdef CONFIG_STREAMOO_CODEC_FIXTURE_TEST
#include "streamoo_decode_stream.h"
#include "streamoo_pcm_adapter.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
extern const uint8_t fixture_start[] asm("_binary_channels_48k_stereo_mp3_start");
extern const uint8_t fixture_end[] asm("_binary_channels_48k_stereo_mp3_end");
static const streamoo_codec_t kind=STREAMOO_CODEC_MP3;
static const char *name="MP3";
#define MAX_REFERENCE_FRAMES 194688u
#else
extern const uint8_t fixture_start[] asm("_binary_channels_48k_stereo_aac_start");
extern const uint8_t fixture_end[] asm("_binary_channels_48k_stereo_aac_end");
static const streamoo_codec_t kind=STREAMOO_CODEC_AAC_LC;
static const char *name="AAC-LC";
#define MAX_REFERENCE_FRAMES 194560u
#endif
static streamoo_decoder_t *decoder;
static streamoo_decode_stream_t *source;
static uint32_t logged_cycles;
static uint32_t max_read_us;
static esp_err_t decode(void *context,uint8_t *input,size_t bytes,bool eos,
    uint8_t *pcm,size_t capacity,streamoo_decode_result_t *result)
{
    return streamoo_decoder_process(context,input,bytes,eos,pcm,capacity,result);
}
static esp_err_t reset(void *context) { return streamoo_decoder_reset(context); }
esp_err_t streamoo_codec_fixture_prepare(void)
{
    if (source || decoder) return ESP_ERR_INVALID_STATE;
    source=heap_caps_calloc(1,sizeof(*source),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if (!source) return ESP_ERR_NO_MEM;
    esp_err_t error=streamoo_decoder_open(kind,&decoder);
    if (error==ESP_OK && !streamoo_decode_stream_init(source,fixture_start,fixture_end-fixture_start,
        decoder,decode,reset,192000,MAX_REFERENCE_FRAMES)) error=ESP_FAIL;
    ESP_LOGI("codec_fixture","%s streaming source %s: input=%u work_bytes=%u; no full PCM clip",
        name,esp_err_to_name(error),(unsigned)(fixture_end-fixture_start),(unsigned)sizeof(*source));
    if (error!=ESP_OK) streamoo_codec_fixture_release();
    return error;
}
bool streamoo_codec_fixture_read(int32_t *lr,size_t frames)
{
    if (!source) return false;
    int64_t start=esp_timer_get_time();
    bool ok=streamoo_decode_stream_read(source,lr,frames);
    uint32_t elapsed=(uint32_t)(esp_timer_get_time()-start);
    if (elapsed>max_read_us) max_read_us=elapsed;
    if (!ok || source->cycles!=logged_cycles) {
        ESP_LOGI("codec_fixture","%s stream ok=%u cycles=%" PRIu32 " calls=%" PRIu32
            " frames=%" PRIu32 " max_read_us=%" PRIu32 " stack_hwm=%u",
            name,(unsigned)ok,source->cycles,source->calls,source->last_cycle_frames,max_read_us,
            (unsigned)uxTaskGetStackHighWaterMark(NULL));
        logged_cycles=source->cycles;
    }
    return ok;
}
void streamoo_codec_fixture_release(void)
{
    streamoo_decoder_close(decoder); decoder=NULL;
    heap_caps_free(source); source=NULL;
    logged_cycles=0; max_read_us=0;
}

#else
esp_err_t streamoo_codec_fixture_prepare(void) { return ESP_ERR_NOT_SUPPORTED; }
bool streamoo_codec_fixture_read(int32_t *lr,size_t frames) { (void)lr; (void)frames; return false; }
void streamoo_codec_fixture_release(void) {}
#endif
