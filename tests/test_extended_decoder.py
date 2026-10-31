"""Real adapter against actual pinned vendor headers; stub binary calls only."""
import os
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include "streamoo_decoder.h"
#include "esp_audio_simple_dec.h"
#include "esp_aac_dec.h"
#include "impl/esp_ts_dec.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned selected,ogg;static int fault;static bool bootstrap,frame_mode;
static esp_audio_err_t reg(unsigned kind){selected=kind;return fault==1?ESP_AUDIO_ERR_FAIL:ESP_AUDIO_ERR_OK;}
esp_audio_err_t esp_mp3_dec_register(void){return reg(0);}
esp_audio_err_t esp_aac_dec_register(void){return reg(1);}
esp_audio_err_t esp_flac_dec_register(void){return reg(2);}
esp_audio_err_t esp_vorbis_dec_register(void){return reg(3);}
esp_audio_err_t esp_opus_dec_register(void){return reg(4);}
esp_audio_err_t esp_ts_dec_register(void){return reg(5);}
esp_audio_err_t esp_ogg_dec_register(void){++ogg;return fault==2?ESP_AUDIO_ERR_FAIL:ESP_AUDIO_ERR_OK;}
esp_audio_err_t esp_audio_simple_dec_open(esp_audio_simple_dec_cfg_t *cfg,void **out){
 assert(cfg->dec_type==(selected==0?ESP_AUDIO_SIMPLE_DEC_TYPE_MP3:selected==1?ESP_AUDIO_SIMPLE_DEC_TYPE_AAC:selected==2?ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC:selected==5?ESP_AUDIO_SIMPLE_DEC_TYPE_TS:ESP_AUDIO_SIMPLE_DEC_TYPE_OGG));
 if(selected==1){esp_aac_dec_cfg_t *aac=cfg->dec_cfg;assert(aac&&aac->aac_plus_enable&&!aac->no_adts_header&&aac->bits_per_sample==16);}
 else if(selected==5){esp_ts_dec_cfg_t *ts=cfg->dec_cfg;assert(ts&&ts->aac_plus_enable);}
 else assert(!cfg->dec_cfg&&!cfg->cfg_size);
 frame_mode=cfg->use_frame_dec;
 if(fault==3)return ESP_AUDIO_ERR_MEM_LACK;
 *out=(void *)1;return ESP_AUDIO_ERR_OK;
}
esp_audio_err_t esp_audio_simple_dec_process(void *h,esp_audio_simple_dec_raw_t *raw,esp_audio_simple_dec_out_t *out){
 assert(h&&raw->len&&out->len>=8);raw->consumed=raw->len;out->decoded_size=8;
 if(selected==2&&frame_mode){assert(raw->len==12&&raw->buffer[0]==255&&raw->buffer[1]==248);bootstrap=true;}
 else if(selected==2&&!bootstrap&&(fault<3||fault>=7)){assert(raw->len==4&&!memcmp(raw->buffer,"fLaC",4));bootstrap=true;out->decoded_size=0;}
 out->needed_size=8;
 if(fault==4){out->needed_size=100000;return ESP_AUDIO_ERR_BUFF_NOT_ENOUGH;}
 if(fault==5)out->decoded_size=out->len+1;
 if(fault==6)raw->consumed=raw->len+1;
 return ESP_AUDIO_ERR_OK;
}
esp_audio_err_t esp_audio_simple_dec_get_info(void *h,esp_audio_simple_dec_info_t *i){assert(h);*i=(esp_audio_simple_dec_info_t){.sample_rate=48000,.channel=2,.bits_per_sample=32};return ESP_AUDIO_ERR_OK;}
esp_audio_err_t esp_audio_simple_dec_reset(void *h){assert(h);bootstrap=false;return ESP_AUDIO_ERR_OK;}
void esp_audio_simple_dec_close(void *h){assert(h);}
int main(int argc,char **argv){assert(argc==3);unsigned kind=(unsigned)atoi(argv[1]);fault=atoi(argv[2]);streamoo_decoder_t *d=NULL;
 assert(streamoo_decoder_open(STREAMOO_CODEC_COUNT,&d)==ESP_ERR_INVALID_ARG&&!d);
 esp_err_t e=streamoo_decoder_open((streamoo_codec_t)kind,&d);
 if(fault==1||(fault==2&&(kind==3||kind==4))||fault==3){assert(e!=ESP_OK&&!d);return 0;}
 assert(e==ESP_OK&&d&&selected==kind&&ogg==((kind==3||kind==4)));
 if(kind==2&&fault>=7){
 uint8_t data[]={255,248,89,172,247,190,157,155,0,1,0,0},pcm[65536];streamoo_decode_result_t r;
 unsigned crc=0;for(unsigned i=0;i<10;++i){crc^=(unsigned)data[i]<<8;for(unsigned k=0;k<8;++k)crc=((crc<<1)^((crc&32768)?0x8005:0))&65535;}data[10]=crc>>8;data[11]=crc;
 for(unsigned cycle=0;cycle<2;++cycle){
 if(fault==10){uint8_t batch[60];for(unsigned i=0;i<5;++i)memcpy(batch+i*12,data,12);unsigned offset=0,outputs=0;while(offset<60){assert(streamoo_decoder_process(d,batch+offset,60-offset,true,pcm,sizeof pcm,&r)==ESP_OK&&r.produced==8&&r.consumed);offset+=r.consumed;++outputs;}assert(outputs==5);}
 else if(fault==9){uint8_t pair[24];memcpy(pair,data,12);memcpy(pair+12,data,12);assert(streamoo_decoder_process(d,pair,24,true,pcm,sizeof pcm,&r)==ESP_OK&&r.produced==8&&r.consumed==21);assert(streamoo_decoder_process(d,pair+21,3,true,pcm,sizeof pcm,&r)==ESP_OK&&r.produced==8&&r.consumed==3);}
 else if(fault==7){assert(streamoo_decoder_process(d,data,sizeof data,true,pcm,sizeof pcm,&r)==ESP_OK&&r.consumed==12&&r.produced==8);}
 else for(unsigned i=0;i<sizeof data;++i){assert(streamoo_decoder_process(d,data+i,1,i==11,pcm,sizeof pcm,&r)==ESP_OK&&r.consumed==1&&r.produced==(i==11?8u:0u));}
 assert(bootstrap&&frame_mode);assert(streamoo_decoder_reset(d)==ESP_OK);}
 streamoo_decoder_close(d);return 0;
 }
 uint8_t input[5]={0},pcm[16];if(kind==2)memcpy(input,"fLaC",4);streamoo_decode_result_t r;
 e=streamoo_decoder_process(d,input,sizeof input,false,pcm,sizeof pcm,&r);
 if(fault==4)assert(e==ESP_ERR_INVALID_SIZE&&r.needed==100000);
 else if(fault==5||fault==6)assert(e==ESP_ERR_INVALID_RESPONSE);
 else assert(e==ESP_OK&&r.format.bits==32&&r.produced==8&&r.consumed==5);
 assert(streamoo_decoder_reset(d)==ESP_OK);streamoo_decoder_close(d);streamoo_decoder_close(NULL);return 0;
}
'''
class ExtendedDecoderTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  (base/'esp_err.h').write_text('typedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_NO_MEM 2\n#define ESP_ERR_INVALID_SIZE 3\n#define ESP_ERR_INVALID_RESPONSE 4\n')
  (base/'esp_log.h').write_text('#include <stdio.h>\n#define ESP_LOGE(tag,...) ((void)printf(__VA_ARGS__))\n#define ESP_LOGI(tag,...) ((void)printf(__VA_ARGS__))\n')
  (base/'test.c').write_text(HARNESS);cls.binary=base/'test'
  includes=['-I'+str(base),'-Icomponents/decoder/include','-Icomponents/pcm_adapter/include']
  vendor=ROOT/'managed_components/espressif__esp_audio_codec/include'
  includes += ['-I'+str(vendor/p) for p in ['.','decoder','decoder/impl','simple_dec']]
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_DECODER_SANITIZE','').split(),*includes,'components/decoder/streamoo_decoder.c','components/decoder/streamoo_flac_header.c','components/decoder/streamoo_flac_frames.c',str(base/'test.c'),'-o',str(cls.binary)],cwd=ROOT,check=True)
 def test_all_codecs_and_bounded_failures(self):
  for codec in range(6):
   for fault in range(7):
    with self.subTest(codec=codec,fault=fault):subprocess.run([str(self.binary),str(codec),str(fault)],check=True)

 def test_headerless_flac_single_frame_fragmentation_and_reset(self):
  for mode in (7,8,9,10):
   with self.subTest(mode=mode):subprocess.run([str(self.binary),'2',str(mode)],check=True)
