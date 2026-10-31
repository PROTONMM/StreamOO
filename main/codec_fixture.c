#include "codec_fixture.h"
#include "sdkconfig.h"
#ifdef CONFIG_STREAMOO_CODEC_FIXTURE_TEST
#include "streamoo_decode_stream.h"
#include "streamoo_buffered_source.h"
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
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
static streamoo_buffered_source_t *compressed;
static uint8_t *compressed_storage;
static TaskHandle_t reader;
static SemaphoreHandle_t reader_done;
static uint32_t source_waits;
static void reader_task(void *context)
{
    (void)context;
    uint32_t generation=0; size_t offset=0;
    bool finished=false;
    uint8_t chunk[997];
    for (;;) {
        streamoo_buffered_source_stats_t stats;
        if (streamoo_buffered_source_stats(compressed,&stats)) {
            if (stats.stopped) break;
            if (generation!=stats.generation) {
                generation=stats.generation; offset=0; finished=false;
            }
            const size_t total=fixture_end-fixture_start;
            for (unsigned burst=0;burst<8 && !finished;++burst) {
                if (offset==total) {
                    finished=streamoo_buffered_source_finish(compressed,generation,false);
                    if (finished) ESP_LOGI("fixture_reader","generation=%" PRIu32 " input=%u stack_hwm=%u",
                        generation,(unsigned)offset,(unsigned)uxTaskGetStackHighWaterMark(NULL));
                    break;
                }
                size_t bytes=total-offset; if (bytes>sizeof chunk) bytes=sizeof chunk;
                /* Future reader I/O goes outside the queue mutex. The token
                 * rejects data if rewind happens while the chunk is prepared. */
                memcpy(chunk,fixture_start+offset,bytes);
                size_t accepted=streamoo_buffered_source_write(compressed,generation,chunk,bytes);
                offset+=accepted;
                if (accepted<bytes) break;
            }
        }
        ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(10) ? pdMS_TO_TICKS(10) : 1);
    }
    /* No shared state accesses after completion acknowledgement. */
    xSemaphoreGive(reader_done);
    vTaskDelete(NULL);
}
static bool rewind_source(void *context)
{
    bool ok=streamoo_buffered_source_rewind(context);
    if (ok) xTaskNotifyGive(reader);
    return ok;
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
    if (source || decoder || reader) return ESP_ERR_INVALID_STATE;
    source=heap_caps_calloc(1,sizeof(*source),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    compressed_storage=heap_caps_malloc(COMPRESSED_BYTES,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if (!source || !compressed_storage) { streamoo_codec_fixture_release(); return ESP_ERR_NO_MEM; }
    compressed=streamoo_buffered_source_create(compressed_storage,COMPRESSED_BYTES);
    reader_done=xSemaphoreCreateBinary();
    if (!compressed || !reader_done) { streamoo_codec_fixture_release(); return ESP_ERR_NO_MEM; }
    esp_err_t error=streamoo_decoder_open(kind,&decoder);
    if (error==ESP_OK && !streamoo_decode_stream_init(source,
        (streamoo_byte_source_t){compressed,streamoo_buffered_source_read,rewind_source},
        decoder,decode,reset,192000,MAX_REFERENCE_FRAMES)) error=ESP_FAIL;
    if (error==ESP_OK && xTaskCreate(reader_task,"fixture_reader",4096,NULL,3,&reader)!=pdPASS) {
        reader=NULL; error=ESP_ERR_NO_MEM;
    }
    ESP_LOGI("codec_fixture","%s streaming source %s: input=%u work_bytes=%u compressed_bytes=%u; independent reader",
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
        streamoo_buffered_source_stats_t stats={0};
        bool stats_ok=streamoo_buffered_source_stats(compressed,&stats);
        ESP_LOGI("codec_fixture","%s stream ok=%u cycles=%" PRIu32 " calls=%" PRIu32
            " frames=%" PRIu32 " compressed=%u backpressure=%" PRIu32 " waits=%" PRIu32 " generation=%" PRIu32 " stale=%" PRIu32 " stats_ok=%u max_read_us=%" PRIu32 " stack_hwm=%u",
            name,(unsigned)(result!=STREAMOO_STREAM_FAILED),source->cycles,source->calls,source->last_cycle_frames,
            (unsigned)stats.queued,stats.backpressure,source_waits,stats.generation,stats.stale,
            (unsigned)stats_ok,max_read_us,
            (unsigned)uxTaskGetStackHighWaterMark(NULL));
        logged_cycles=source->cycles;
    }
    return result;
}
void streamoo_codec_fixture_release(void)
{
    if (reader) {
        if (!streamoo_buffered_source_stop(compressed)) {
            ESP_LOGE("codec_fixture","Reader stop lock timeout; retaining live buffers"); return;
        }
        xTaskNotifyGive(reader);
        if (xSemaphoreTake(reader_done,pdMS_TO_TICKS(1000))!=pdTRUE) {
            ESP_LOGE("codec_fixture","Reader join timeout; retaining live buffers"); return;
        }
        reader=NULL;
    }
    if (reader_done) { vSemaphoreDelete(reader_done); reader_done=NULL; }
    streamoo_decoder_close(decoder); decoder=NULL;
    heap_caps_free(source); source=NULL;
    streamoo_buffered_source_destroy(compressed); compressed=NULL;
    heap_caps_free(compressed_storage); compressed_storage=NULL;
    logged_cycles=0; max_read_us=0; source_waits=0;
}

#else
esp_err_t streamoo_codec_fixture_prepare(void) { return ESP_ERR_NOT_SUPPORTED; }
streamoo_stream_result_t streamoo_codec_fixture_read(int32_t *lr,size_t frames) { (void)lr; (void)frames; return STREAMOO_STREAM_FAILED; }
void streamoo_codec_fixture_release(void) {}
#endif
