#include "streamoo_flac_header.h"
#include <string.h>
#include <stdbool.h>

static streamoo_flac_header_result_t parse_header(const uint8_t *p,size_t n,
    size_t capacity,uint8_t out[42],size_t *header_bytes,bool boundary)
{
    if(!p||!out||!header_bytes||n>16)return STREAMOO_FLAC_BAD;
    *header_bytes=0;
    if(n<4)return STREAMOO_FLAC_MORE;
    if(!memcmp(p,"fLaC",4)){*header_bytes=4;return STREAMOO_FLAC_FILE;}
    if(p[0]!=255 || (p[1]&0xfe)!=0xf8 || (p[3]&1))return STREAMOO_FLAC_BAD;
    unsigned block_code=p[2]>>4,rate_code=p[2]&15,channel_code=p[3]>>4;
    static const unsigned rates[12]={0,88200,176400,192000,8000,16000,22050,24000,32000,44100,48000,96000};
    static const unsigned depths[8]={0,8,12,0,16,20,24,32};
    unsigned bits=depths[(p[3]>>1)&7];
    if(boundary && !((p[3]>>1)&7))bits=16; /* Reference STREAMINFO: structural scan only. */
    unsigned channels=channel_code<=7?channel_code+1:channel_code<=10?2:0;
    if(!block_code||(!rate_code&&!boundary)||rate_code==15||!channels||channels>2||
       (bits!=16&&bits!=24&&bits!=32))return STREAMOO_FLAC_BAD;
    if(n<5)return STREAMOO_FLAC_MORE;
    /* FLAC's coded number extends UTF-8 to seven octets, up to36 bits. */
    unsigned count=1;uint8_t first=p[4];uint64_t number=first;
    if(first&0x80){
        count=0;for(uint8_t mask=0x80;mask && (first&mask);mask>>=1)++count;
        if(count<2||count>7)return STREAMOO_FLAC_BAD;
        number=first&((1u<<(7-count))-1u);
    }
    if(n<4+count)return STREAMOO_FLAC_MORE;
    for(unsigned i=1;i<count;++i){
        if((p[4+i]&0xc0)!=0x80)return STREAMOO_FLAC_BAD;
        number=(number<<6)|(p[4+i]&0x3f);
    }
    static const uint64_t minimum[8]={0,0,128,2048,65536,2097152,67108864,2147483648ULL};
    if(number<minimum[count]||number>((p[1]&1)?UINT64_C(0xfffffffff):UINT64_C(0x7fffffff)))return STREAMOO_FLAC_BAD;
    size_t at=4+count;
    unsigned block=block_code==1?192:block_code<=5?144u<<block_code:block_code>=8?1u<<block_code:0;
    unsigned block_extra=block_code==6?1:block_code==7?2:0;
    unsigned rate_extra=rate_code==12?1:rate_code>=13?2:0;
    size_t length=at+block_extra+rate_extra+1;
    if(length>16)return STREAMOO_FLAC_BAD;
    if(n<length)return STREAMOO_FLAC_MORE;
    if(block_extra){block=p[at++];if(block_extra==2)block=(block<<8)|p[at++];++block;}
    unsigned rate=rate_code<12?rates[rate_code]:p[at++];
    if(rate_extra==2)rate=(rate<<8)|p[at++];
    if(rate_code==12)rate*=1000;
    if(rate_code==14)rate*=10;
    if((!boundary&&(rate<8000||rate>48000||block>capacity/(channels*(bits/8))))||block<16||block>65535)return STREAMOO_FLAC_BAD;
    uint8_t crc=0;
    for(size_t i=0;i<length-1;++i){
        crc^=p[i];
        for(unsigned bit=0;bit<8;++bit)crc=(uint8_t)((crc<<1)^((crc&0x80)?7:0));
    }
    if(crc!=p[length-1])return STREAMOO_FLAC_BAD;
    memset(out,0,42);memcpy(out,"fLaC\x80\0\0\x22",8);
    unsigned min_block=(p[1]&1)?16:block;
    unsigned max_block=(p[1]&1)?(unsigned)(capacity/(channels*(bits/8))):block;
    if(max_block>65535)max_block=65535;
    out[8]=(uint8_t)(min_block>>8);out[9]=(uint8_t)min_block;
    out[10]=(uint8_t)(max_block>>8);out[11]=(uint8_t)max_block;
    /* Zero makes Espressif2.6.2 choose a20KiB parser window, too small for
     * live24-bit stereo. Declare our supported upper bound, not a measured
     * frame length or a bitrate estimate. Larger frames remain unsupported. */
    out[15]=(uint8_t)(STREAMOO_FLAC_MAX_FRAME_BYTES>>16);
    out[16]=(uint8_t)(STREAMOO_FLAC_MAX_FRAME_BYTES>>8);
    out[17]=(uint8_t)STREAMOO_FLAC_MAX_FRAME_BYTES;
    uint64_t info=((uint64_t)rate<<44)|((uint64_t)(channels-1)<<41)|((uint64_t)(bits-1)<<36);
    for(unsigned i=0;i<8;++i)out[18+i]=(uint8_t)(info>>(56-8*i));
    /* Unknown minimum frame size, sample count and MD5 stay zero. */
    *header_bytes=length;
    return STREAMOO_FLAC_FRAME;
}

streamoo_flac_header_result_t streamoo_flac_header(const uint8_t *p,size_t n,size_t capacity,uint8_t out[42],size_t *bytes){
    return parse_header(p,n,capacity,out,bytes,false);
}
streamoo_flac_header_result_t streamoo_flac_boundary_header(const uint8_t *p,size_t n,size_t *bytes){
    uint8_t unused[42];return parse_header(p,n,SIZE_MAX,unused,bytes,true);
}
