#include "streamoo_mp3_decoder.h"
#include "esp_mp3_dec.h"
#include "esp_log.h"
#include "esp_audio_simple_dec.h"
#include <stdlib.h>
#include <limits.h>

struct streamoo_mp3_decoder { esp_audio_simple_dec_handle_t handle; };
static bool registered;
esp_err_t streamoo_mp3_open(streamoo_mp3_decoder_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out=NULL;
    if (!registered) {
        esp_audio_err_t e=esp_mp3_dec_register();
        if (e!=ESP_AUDIO_ERR_OK && e!=ESP_AUDIO_ERR_ALREADY_EXIST) {
            ESP_LOGE("mp3_decoder","Register failed: vendor=%d",(int)e);
            return ESP_FAIL;
        }
        registered=true;
    }
    streamoo_mp3_decoder_t *d=calloc(1,sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;
    esp_audio_simple_dec_cfg_t config={.dec_type=ESP_AUDIO_SIMPLE_DEC_TYPE_MP3};
    esp_audio_err_t error=esp_audio_simple_dec_open(&config,&d->handle);
    if (error!=ESP_AUDIO_ERR_OK) {
        ESP_LOGE("mp3_decoder","Open failed: vendor=%d",(int)error);
        free(d); return error==ESP_AUDIO_ERR_MEM_LACK ? ESP_ERR_NO_MEM : ESP_FAIL;
    }
    *out=d; return ESP_OK;
}
esp_err_t streamoo_mp3_process(streamoo_mp3_decoder_t *d,uint8_t *input,size_t bytes,
    bool eos,uint8_t *pcm,size_t capacity,streamoo_mp3_result_t *result)
{
    if (!d || !input || !bytes || !pcm || !capacity || !result || bytes>UINT32_MAX || capacity>UINT32_MAX)
        return ESP_ERR_INVALID_ARG;
    *result=(streamoo_mp3_result_t){0};
    esp_audio_simple_dec_raw_t raw={.buffer=input,.len=bytes,.eos=eos};
    esp_audio_simple_dec_out_t out={.buffer=pcm,.len=capacity};
    esp_audio_err_t error=esp_audio_simple_dec_process(d->handle,&raw,&out);
    result->codec_status=(int32_t)error;
    result->consumed=raw.consumed; result->produced=out.decoded_size; result->needed=out.needed_size;
    if (result->consumed>bytes || result->produced>capacity) return ESP_ERR_INVALID_RESPONSE;
    if (error==ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) return ESP_ERR_INVALID_SIZE;
    if (error!=ESP_AUDIO_ERR_OK) return ESP_FAIL;
    if (out.decoded_size) {
        esp_audio_simple_dec_info_t info={0};
        if (esp_audio_simple_dec_get_info(d->handle,&info)!=ESP_AUDIO_ERR_OK) return ESP_ERR_INVALID_RESPONSE;
        result->format=(streamoo_pcm_format_t){info.sample_rate,info.channel,info.bits_per_sample};
    }
    return ESP_OK;
}
void streamoo_mp3_close(streamoo_mp3_decoder_t *d)
{
    if (d) { esp_audio_simple_dec_close(d->handle); free(d); }
}
