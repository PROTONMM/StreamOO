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
#define COMPRESSED_BYTES 4096u
static streamoo_byte_ring_t compressed;
static uint8_t *compressed_storage;
static size_t feed_offset;
static uint32_t backpressure, source_waits;
void streamoo_codec_fixture_service(void)
{
    if (!compressed_storage || compressed.ended) return;
    size_t total=fixture_end-fixture_start;
    while (feed_offset<total) {
        size_t bytes=total-feed_offset;
        if (bytes>997) bytes=997;
        size_t written=streamoo_byte_ring_write(&compressed,fixture_start+feed_offset,bytes);
        feed_offset+=written;
        if (written<bytes) { ++backpressure; return; }
    }
    streamoo_byte_ring_finish(&compressed,false);
}
static bool rewind_source(void *context)
{
    streamoo_byte_ring_clear(context); feed_offset=0;
    streamoo_codec_fixture_service(); return true;
}
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
    compressed_storage=heap_caps_malloc(COMPRESSED_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!source || !compressed_storage) { streamoo_codec_fixture_release(); return ESP_ERR_NO_MEM; }
    streamoo_byte_ring_init(&compressed,compressed_storage,COMPRESSED_BYTES);
    streamoo_codec_fixture_service();
    esp_err_t error=streamoo_decoder_open(kind,&decoder);
    if (error==ESP_OK && !streamoo_decode_stream_init(source,
        (streamoo_byte_source_t){&compressed,streamoo_byte_ring_read,rewind_source},
        decoder,decode,reset,192000,MAX_REFERENCE_FRAMES)) error=ESP_FAIL;
    ESP_LOGI("codec_fixture","%s streaming source %s: input=%u work_bytes=%u compressed_bytes=%u; buffered source",
        name,esp_err_to_name(error),(unsigned)(fixture_end-fixture_start),(unsigned)sizeof(*source),COMPRESSED_BYTES);
    if (error!=ESP_OK) streamoo_codec_fixture_release();
    return error;
}
streamoo_stream_result_t streamoo_codec_fixture_read(int32_t *lr,size_t frames)
{
    if (!source) return STREAMOO_STREAM_FAILED;
    int64_t start=esp_timer_get_time();
    streamoo_stream_result_t result=streamoo_decode_stream_read(source,lr,frames);
    uint32_t elapsed=(uint32_t)(esp_timer_get_time()-start);
    if (elapsed>max_read_us) max_read_us=elapsed;
    if (result==STREAMOO_STREAM_WAIT) ++source_waits;
    if (result==STREAMOO_STREAM_FAILED || source->cycles!=logged_cycles) {
        ESP_LOGI("codec_fixture","%s stream ok=%u cycles=%" PRIu32 " calls=%" PRIu32
            " frames=%" PRIu32 " compressed=%u backpressure=%" PRIu32 " waits=%" PRIu32 " max_read_us=%" PRIu32 " stack_hwm=%u",
            name,(unsigned)(result!=STREAMOO_STREAM_FAILED),source->cycles,source->calls,source->last_cycle_frames,
            (unsigned)compressed.count,backpressure,source_waits,max_read_us,
            (unsigned)uxTaskGetStackHighWaterMark(NULL));
        logged_cycles=source->cycles;
    }
    return result;
}
void streamoo_codec_fixture_release(void)
{
    streamoo_decoder_close(decoder); decoder=NULL;
    heap_caps_free(source); source=NULL;
    heap_caps_free(compressed_storage); compressed_storage=NULL;
    compressed=(streamoo_byte_ring_t){0}; feed_offset=0;
    logged_cycles=0; max_read_us=0; backpressure=0; source_waits=0;
}

#else
void streamoo_codec_fixture_service(void) {}
esp_err_t streamoo_codec_fixture_prepare(void) { return ESP_ERR_NOT_SUPPORTED; }
streamoo_stream_result_t streamoo_codec_fixture_read(int32_t *lr,size_t frames) { (void)lr; (void)frames; return STREAMOO_STREAM_FAILED; }
void streamoo_codec_fixture_release(void) {}
#endif
