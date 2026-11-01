#include "streamoo_flac_parse.h"
#include "streamoo_flac_header.h"
#include <string.h>

typedef struct {
    size_t scan;
    uint32_t metadata_left;
    uint16_t crc;
    bool metadata,last_metadata;
} flac_parse_t;

/* Original nibble table from polynomial x^16+x^15+x^2+1, MSB first. */
static uint16_t advance(uint16_t crc,uint8_t value)
{
    static const uint16_t table[16]={0x0000,0x8005,0x800f,0x000a,
        0x801b,0x001e,0x0014,0x8011,0x8033,0x0036,0x003c,0x8039,
        0x0028,0x802d,0x8027,0x0022};
    crc^=(uint16_t)value<<8;
    crc=(uint16_t)((crc<<4)^table[crc>>12]);
    return (uint16_t)((crc<<4)^table[crc>>12]);
}

esp_es_parse_err_t streamoo_flac_parse(esp_es_parse_raw_t *in,esp_es_parse_frame_info_t *info)
{
    if(!in||!info||!in->buffer)return ESP_ES_PARSE_ERR_INVALID_ARG;
    const uint8_t *p=in->buffer;size_t n=in->len;
    info->frame_size=info->skipped_size=0;
    flac_parse_t *s=info->extra_data;
    if(in->bos){
        if(n<42)return ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH;
        if(memcmp(p,"fLaC",4)||(p[4]&127)||p[5]||p[6]||p[7]!=34)
            return ESP_ES_PARSE_ERR_NOT_CONTINUE;
        unsigned rate=((unsigned)p[18]<<12)|((unsigned)p[19]<<4)|(p[20]>>4);
        unsigned channels=((p[20]>>1)&7)+1,bits=(((p[20]&1)<<4)|(p[21]>>4))+1;
        if(rate<8000||rate>48000||channels>2||(bits!=16&&bits!=24&&bits!=32))
            return ESP_ES_PARSE_ERR_NOT_SUPPORT;
        if(!s){s=calloc(1,sizeof *s);if(!s)return ESP_ES_PARSE_ERR_NO_MEM;info->extra_data=s;}
        *s=(flac_parse_t){.metadata=!(p[4]&128)};
        info->aud_info.sample_rate=(int)rate;info->aud_info.channel=(uint8_t)channels;
        info->aud_info.bits_per_sample=(uint8_t)bits;
        info->max_frame_size=STREAMOO_FLAC_MAX_FRAME_BYTES+16;
        info->skipped_size=42;
        return ESP_ES_PARSE_ERR_OK;
    }
    if(!s)return ESP_ES_PARSE_ERR_INVALID_ARG;
    if(!n)return ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH;
    /* File metadata can be larger than the audio cache (e.g. cover art). Skip
     * payload incrementally, never ask the SDK to cache the whole block. */
    if(s->metadata){
        size_t used=0;
        if(!s->metadata_left){
            if(n<4)return ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH;
            unsigned type=p[0]&127;
            if(!type||type==127)return ESP_ES_PARSE_ERR_WRONG_HEADER;
            s->last_metadata=(p[0]&128)!=0;
            s->metadata_left=((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];used=4;
        }
        size_t payload=n-used;if(payload>s->metadata_left)payload=s->metadata_left;
        s->metadata_left-=(uint32_t)payload;info->skipped_size=(uint32_t)(used+payload);
        if(!s->metadata_left&&s->last_metadata)s->metadata=false;
        return ESP_ES_PARSE_ERR_OK;
    }
    size_t header=0,available=n>16?16:n;
    streamoo_flac_header_result_t first=streamoo_flac_boundary_header(p,available,&header);
    if(first==STREAMOO_FLAC_MORE)return in->eos?ESP_ES_PARSE_ERR_WRONG_HEADER:ESP_ES_PARSE_ERR_DATA_NOT_ENOUGH;
    if(first!=STREAMOO_FLAC_FRAME)return ESP_ES_PARSE_ERR_WRONG_HEADER;
    /* Reparse on arrival of new input, NOT only after filling max_frame_size.
     * scan/crc refer to the SDK's unchanged cached prefix until a frame is returned. */
    if(s->scan>n)return ESP_ES_PARSE_ERR_FAIL;
    while(s->scan+4<=n){
        size_t at=s->scan;
        if(at>=header+2&&!s->crc&&p[at]==255&&(p[at+1]&0xfe)==0xf8){
            size_t next=0,left=n-at;if(left>16)left=16;
            streamoo_flac_header_result_t r=streamoo_flac_boundary_header(p+at,left,&next);
            if(r==STREAMOO_FLAC_MORE&&!in->eos)break;
            if(r==STREAMOO_FLAC_FRAME){
                info->frame_size=(uint32_t)at;s->scan=0;s->crc=0;
                return ESP_ES_PARSE_ERR_OK;
            }
        }
        if(at>=STREAMOO_FLAC_MAX_FRAME_BYTES)return ESP_ES_PARSE_ERR_NOT_SUPPORT;
        s->crc=advance(s->crc,p[s->scan++]);
    }
    if(in->eos){
        if(n>STREAMOO_FLAC_MAX_FRAME_BYTES||n<header+2)return ESP_ES_PARSE_ERR_WRONG_HEADER;
        while(s->scan<n)s->crc=advance(s->crc,p[s->scan++]);
        if(s->crc)return ESP_ES_PARSE_ERR_WRONG_HEADER;
        info->frame_size=(uint32_t)n;s->scan=0;
        return ESP_ES_PARSE_ERR_OK;
    }
    if(n>=STREAMOO_FLAC_MAX_FRAME_BYTES+16)return ESP_ES_PARSE_ERR_NOT_SUPPORT;
    info->frame_size=ES_PARSE_REPARSE_FRAME_SIZE;
    return ESP_ES_PARSE_ERR_OK;
}
