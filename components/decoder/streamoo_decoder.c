#include "streamoo_decoder.h"
#include "esp_mp3_dec.h"
#include "esp_aac_dec.h"
#include "esp_flac_dec.h"
#include "esp_vorbis_dec.h"
#include "esp_opus_dec.h"
#include "impl/esp_ogg_dec.h"
#include "esp_log.h"
#include "esp_audio_simple_dec.h"
#include <stdlib.h>
#include <limits.h>

struct streamoo_decoder { esp_audio_simple_dec_handle_t handle; esp_aac_dec_cfg_t aac; };
static bool registered[STREAMOO_CODEC_COUNT];
static bool ogg_registered;
esp_err_t streamoo_decoder_open(streamoo_codec_t codec, streamoo_decoder_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out=NULL;
    if (codec<STREAMOO_CODEC_MP3 || codec>=STREAMOO_CODEC_COUNT) return ESP_ERR_INVALID_ARG;
    if (!registered[codec]) {
        esp_audio_err_t e;
        switch(codec){
        case STREAMOO_CODEC_MP3:e=esp_mp3_dec_register();break;
        case STREAMOO_CODEC_AAC_LC:e=esp_aac_dec_register();break;
        case STREAMOO_CODEC_FLAC:e=esp_flac_dec_register();break;
        case STREAMOO_CODEC_VORBIS:e=esp_vorbis_dec_register();break;
        case STREAMOO_CODEC_OPUS:e=esp_opus_dec_register();break;
        default:return ESP_ERR_INVALID_ARG;
        }
        if (e!=ESP_AUDIO_ERR_OK && e!=ESP_AUDIO_ERR_ALREADY_EXIST) {
            ESP_LOGE("decoder","Register failed: vendor=%d",(int)e);
            return ESP_FAIL;
        }
        registered[codec]=true;
    }
    if ((codec==STREAMOO_CODEC_VORBIS || codec==STREAMOO_CODEC_OPUS) && !ogg_registered) {
        esp_audio_err_t e=esp_ogg_dec_register();
        if(e!=ESP_AUDIO_ERR_OK && e!=ESP_AUDIO_ERR_ALREADY_EXIST)return ESP_FAIL;
        ogg_registered=true;
    }
    streamoo_decoder_t *d=calloc(1,sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;
    esp_audio_simple_dec_cfg_t config={.dec_type=codec==STREAMOO_CODEC_MP3 ?
        ESP_AUDIO_SIMPLE_DEC_TYPE_MP3 : codec==STREAMOO_CODEC_AAC_LC ? ESP_AUDIO_SIMPLE_DEC_TYPE_AAC :
        codec==STREAMOO_CODEC_FLAC ? ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC : ESP_AUDIO_SIMPLE_DEC_TYPE_OGG};
    if (codec==STREAMOO_CODEC_AAC_LC) {
        d->aac=(esp_aac_dec_cfg_t)ESP_AAC_DEC_CONFIG_DEFAULT();
        d->aac.channel=2; d->aac.no_adts_header=false; d->aac.aac_plus_enable=true;
        config.dec_cfg=&d->aac; config.cfg_size=sizeof d->aac;
    }
    esp_audio_err_t error=esp_audio_simple_dec_open(&config,&d->handle);
    if (error!=ESP_AUDIO_ERR_OK) {
        ESP_LOGE("decoder","Open failed: vendor=%d",(int)error);
        free(d); return error==ESP_AUDIO_ERR_MEM_LACK ? ESP_ERR_NO_MEM : ESP_FAIL;
    }
    *out=d; return ESP_OK;
}
esp_err_t streamoo_decoder_process(streamoo_decoder_t *d,uint8_t *input,size_t bytes,
    bool eos,uint8_t *pcm,size_t capacity,streamoo_decode_result_t *result)
{
    if (!d || !input || !bytes || !pcm || !capacity || !result || bytes>UINT32_MAX || capacity>UINT32_MAX)
        return ESP_ERR_INVALID_ARG;
    *result=(streamoo_decode_result_t){0};
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
esp_err_t streamoo_decoder_reset(streamoo_decoder_t *d)
{
    if (!d) return ESP_ERR_INVALID_ARG;
    return esp_audio_simple_dec_reset(d->handle)==ESP_AUDIO_ERR_OK ? ESP_OK : ESP_FAIL;
}
void streamoo_decoder_close(streamoo_decoder_t *d)
{
    if (d) { esp_audio_simple_dec_close(d->handle); free(d); }
}
