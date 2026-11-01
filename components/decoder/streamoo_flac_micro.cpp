#include "streamoo_flac_alt.h"
#include "micro_flac/flac_decoder.h"
#include <new>

struct streamoo_flac_alt { micro_flac::FLACDecoder decoder; };
extern "C" esp_err_t streamoo_flac_alt_open(streamoo_flac_alt_t **out) {
    *out=new(std::nothrow) streamoo_flac_alt;
    if(!*out)return ESP_ERR_NO_MEM;
    (*out)->decoder.set_crc_check_enabled(true);
    return ESP_OK;
}
extern "C" esp_err_t streamoo_flac_alt_process(streamoo_flac_alt_t *s,uint8_t *p,size_t n,bool eos,
    uint8_t *pcm,size_t cap,streamoo_decode_result_t *r) {
    *r={};
    for(unsigned step=0;step<8;++step){
        size_t used=0,samples=0;
        auto status=s->decoder.decode(p+r->consumed,n-r->consumed,
            reinterpret_cast<int32_t *>(pcm),cap/4,used,samples);
        r->codec_status=status;r->consumed+=used;
        if(r->consumed>n||samples>cap/4)return ESP_ERR_INVALID_RESPONSE;
        if(status<0){
            if(status==micro_flac::FLAC_DECODER_ERROR_OUTPUT_TOO_SMALL){
                const auto &info=s->decoder.get_stream_info();
                r->needed=(size_t)info.max_block_size()*info.num_channels()*4;return ESP_ERR_INVALID_SIZE;
            }
            return status==micro_flac::FLAC_DECODER_ERROR_MEMORY_ALLOCATION?ESP_ERR_NO_MEM:ESP_FAIL;
        }
        if(samples){
            const auto &info=s->decoder.get_stream_info();
            r->produced=samples*4;r->format={info.sample_rate(),static_cast<uint8_t>(info.num_channels()),32};
            return ESP_OK;
        }
        if(status==micro_flac::FLAC_DECODER_HEADER_READY&&r->consumed<n)continue;
        if(eos&&status==micro_flac::FLAC_DECODER_NEED_MORE_DATA)return ESP_ERR_INVALID_RESPONSE;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_RESPONSE;
}
extern "C" esp_err_t streamoo_flac_alt_reset(streamoo_flac_alt_t *s) {
    s->decoder.reset();return ESP_OK;
}
extern "C" void streamoo_flac_alt_close(streamoo_flac_alt_t *s) { delete s; }
