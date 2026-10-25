#include "mp3_fixture.h"
#include "streamoo_mp3_decoder.h"
#include "streamoo_pcm_adapter.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <inttypes.h>
extern const uint8_t fixture_start[] asm("_binary_channels_48k_stereo_mp3_start");
extern const uint8_t fixture_end[] asm("_binary_channels_48k_stereo_mp3_end");
static uint8_t *decoded;
static streamoo_pcm_clip_t source;
#define CLIP_LIMIT (1024u*1024u)
#define SCRATCH_BYTES 8192u

esp_err_t streamoo_mp3_fixture_prepare(void)
{
    if (decoded) return ESP_ERR_INVALID_STATE;
    streamoo_mp3_decoder_t *decoder=NULL;
    uint8_t *clip=heap_caps_malloc(CLIP_LIMIT,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    uint8_t *scratch=heap_caps_malloc(SCRATCH_BYTES,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    if (!clip || !scratch) { heap_caps_free(clip); heap_caps_free(scratch); return ESP_ERR_NO_MEM; }
    const int64_t started=esp_timer_get_time();
    esp_err_t error=streamoo_mp3_open(&decoder);
    size_t file_bytes=fixture_end-fixture_start, offset=0, total=0;
    uint32_t calls=0, peak=0;
    int64_t decode_us=0;
    streamoo_pcm_format_t detected={0};
    while (error==ESP_OK && offset<file_bytes) {
        uint8_t chunk[257];
        size_t length=file_bytes-offset;
        if (length>sizeof chunk) length=sizeof chunk;
        memcpy(chunk,fixture_start+offset,length);
        size_t pending=length;
        uint8_t *input=chunk;
        while (error==ESP_OK && pending) {
            streamoo_mp3_result_t result={0};
            const int64_t t=esp_timer_get_time();
            error=streamoo_mp3_process(decoder,input,pending,offset+length==file_bytes,
                                        scratch,SCRATCH_BYTES,&result);
            decode_us+=esp_timer_get_time()-t;
            if (error!=ESP_OK) {
                ESP_LOGE("mp3_fixture","Decode error %s vendor=%d at offset=%u pending=%u need=%u",
                         esp_err_to_name(error),(int)result.codec_status,(unsigned)offset,(unsigned)pending,(unsigned)result.needed);
                break;
            }
            if ((!result.consumed && !result.produced) || ++calls>10000) { error=ESP_ERR_INVALID_RESPONSE; break; }
            if (result.produced) {
                if (result.format.rate!=48000 || result.format.channels!=2 || result.format.bits!=16 ||
                    result.produced%4 || result.produced>CLIP_LIMIT-total) { error=ESP_ERR_NOT_SUPPORTED; break; }
                detected=result.format;
                memcpy(clip+total,scratch,result.produced); total+=result.produced;
                for (size_t i=0;i<result.produced;i+=2) {
                    uint32_t value=(uint32_t)scratch[i]|((uint32_t)scratch[i+1]<<8);
                    if (value>=32768) value=65536-value;
                    if (value>peak) peak=value;
                }
            }
            pending-=result.consumed; input+=result.consumed;
            if (calls%32==0) vTaskDelay(1);
        }
        offset+=length;
    }
    streamoo_mp3_close(decoder); heap_caps_free(scratch);
    /* Padding/priming can vary by decoder; require all 4s source duration,
     * tolerate at most one extra MP3 frame beyond host reference193536. */
    if (error==ESP_OK && (total/4<192000 || total/4>194688 || !peak || peak>2048)) error=ESP_ERR_INVALID_RESPONSE;
    ESP_LOGI("mp3_fixture","MP3 PoC %s: input=%u pcm_frames=%u rate=%" PRIu32 " channels=%u bits=%u peak16=%" PRIu32 " decode_us=%" PRId64 " wall_us=%" PRId64 " stack_hwm=%u",
             esp_err_to_name(error),(unsigned)file_bytes,(unsigned)(total/4),detected.rate,
             detected.channels,detected.bits,peak,decode_us,esp_timer_get_time()-started,
             (unsigned)uxTaskGetStackHighWaterMark(NULL));
    if (error!=ESP_OK) { heap_caps_free(clip); return error; }
    if (!streamoo_pcm_clip_init(&source,detected,clip,total)) { heap_caps_free(clip); return ESP_ERR_INVALID_RESPONSE; }
    decoded=clip;
    return ESP_OK;
}
bool streamoo_mp3_fixture_read(int32_t *lr,size_t frames)
{
    return decoded && streamoo_pcm_clip_read(&source,lr,frames);
}
void streamoo_mp3_fixture_release(void)
{
    heap_caps_free(decoded); decoded=NULL;
    source=(streamoo_pcm_clip_t){0};
}
