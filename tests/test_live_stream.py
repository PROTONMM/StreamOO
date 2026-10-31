"""Pure ICY framing and live PCM conversion with deterministic decoder/source fakes."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include "streamoo_icy.h"
#include "streamoo_live_decode.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {unsigned calls,frame,rate,channels;bool end,bad;} fake_t;
static streamoo_source_result_t read_source(void *ctx,uint8_t *out,size_t cap,size_t *n,bool *last){
 fake_t *f=ctx;assert(cap);*n=0;*last=false;++f->calls;
 if(f->end)return STREAMOO_SOURCE_END;
 if(f->calls%3==0)return STREAMOO_SOURCE_WAIT;
 out[0]=1;*n=1;return STREAMOO_SOURCE_DATA;
}
static esp_err_t decode(void *ctx,uint8_t *in,size_t n,bool eos,uint8_t *out,size_t cap,streamoo_decode_result_t *r){
 fake_t *f=ctx;(void)in;(void)eos;assert(n&&cap>=17*4);
 for(unsigned i=0;i<17;++i){int16_t a=-20000+(f->frame++)*7;for(unsigned c=0;c<f->channels;++c){int16_t v=c?-a:a;
 unsigned p=(i*f->channels+c)*2;out[p]=(uint8_t)v;out[p+1]=(uint8_t)((uint16_t)v>>8);}}
 *r=(streamoo_decode_result_t){.consumed=1,.produced=17*2*f->channels,.format={f->bad?96000:f->rate,f->channels,16}};return ESP_OK;
}
static void resample(unsigned rate,unsigned channels){
 fake_t fake={.rate=rate,.channels=channels};streamoo_live_decode_t s;
 assert(streamoo_live_decode_init(&s,(streamoo_byte_source_t){&fake,read_source,NULL},&fake,decode));
 unsigned produced=0,waits=0;int32_t output[480],before[480];memset(output,0x5a,sizeof output);
 for(unsigned tries=0;tries<1000&&produced<960;++tries){memcpy(before,output,sizeof output);
 streamoo_stream_result_t r=streamoo_live_decode_read(&s,output);
 if(r==STREAMOO_STREAM_WAIT){++waits;assert(!memcmp(before,output,sizeof output));continue;}
 assert(r==STREAMOO_STREAM_READY);
 for(unsigned i=0;i<240;++i){uint64_t position=(uint64_t)(produced+i)*rate;int64_t whole=position/48000,phase=position%48000;
 int32_t expected=(int32_t)(((-20000+whole*7)*48000+7*phase)*65536/48000);
 assert(output[i*2]==expected);assert(output[i*2+1]==(channels==1?expected:-expected));}
 produced+=240;
 }
 assert(produced==960&&waits);
 fake.bad=true;bool failed=false;for(unsigned i=0;i<100;++i)if(streamoo_live_decode_read(&s,output)==STREAMOO_STREAM_FAILED){failed=true;break;}assert(failed);
 assert(streamoo_live_decode_read(&s,output)==STREAMOO_STREAM_FAILED);
 fake_t ended={.rate=48000,.channels=2,.end=true};assert(streamoo_live_decode_init(&s,(streamoo_byte_source_t){&ended,read_source,NULL},&ended,decode));
 assert(streamoo_live_decode_read(&s,output)==STREAMOO_STREAM_FAILED&&ended.calls==1);
}
static void icy(unsigned chunk){
 streamoo_icy_t s;assert(streamoo_icy_init(&s,3));uint8_t wire[128]={0},audio[128],out[128];size_t n=0,total=0;
 memcpy(wire+n,"abc",3);n+=3;wire[n++]=2;memcpy(wire+n,"StreamTitle='Zażółć';",strlen("StreamTitle='Zażółć';"));n+=32;
 memcpy(wire+n,"def",3);n+=3;wire[n++]=0;memcpy(wire+n,"ghi",3);n+=3;
 for(size_t at=0;at<n;){size_t size=n-at;if(size>chunk)size=chunk;size_t written=0;
 assert(streamoo_icy_feed(&s,wire+at,size,out,sizeof out,&written));memcpy(audio+total,out,written);total+=written;at+=size;}
 assert(total==9&&!memcmp(audio,"abcdefghi",9));assert(!strcmp(s.title,"Zażółć")&&s.revision==1&&s.title_audio_offset==3&&s.audio_total==9);
 assert(streamoo_icy_init(&s,0));size_t written=0;assert(streamoo_icy_feed(&s,(uint8_t *)"raw",3,out,sizeof out,&written)&&written==3&&!memcmp(out,"raw",3));
 assert(!streamoo_icy_init(&s,16777217));assert(!streamoo_icy_feed(&s,(uint8_t *)"raw",3,out,2,&written));
 assert(streamoo_icy_init(&s,1));uint8_t maximum[4083];maximum[0]='a';maximum[1]=255;memset(maximum+2,'x',4080);maximum[4082]='b';
 uint8_t *large=malloc(sizeof maximum);assert(large);assert(streamoo_icy_feed(&s,maximum,sizeof maximum,large,sizeof maximum,&written));assert(written==2&&large[0]=='a'&&large[1]=='b');free(large);
}

typedef struct {unsigned fault,calls;} diagnostic_t;
static streamoo_source_result_t diagnostic_source(void *ctx,uint8_t *out,size_t cap,size_t *n,bool *last){
 diagnostic_t *d=ctx;out[0]=1;*n=0;*last=false;
 if(d->fault==9)return STREAMOO_SOURCE_END;
 if(d->fault==10)return STREAMOO_SOURCE_ERROR;
 if(d->fault==11)return STREAMOO_SOURCE_DATA;
 if(d->fault==12){*n=1;return STREAMOO_SOURCE_WAIT;}
 if(d->fault==13){*n=cap+1;return STREAMOO_SOURCE_DATA;}
 *n=1;*last=d->fault==14;return STREAMOO_SOURCE_DATA;
}
static esp_err_t diagnostic_decode(void *ctx,uint8_t *in,size_t bytes,bool eos,uint8_t *pcm,size_t cap,streamoo_decode_result_t *r){
 diagnostic_t *d=ctx;(void)in;(void)eos;assert(bytes&&cap>=4);++d->calls;memset(pcm,0,4);
 *r=(streamoo_decode_result_t){.consumed=1,.produced=4,.format={44100,2,16}};
 switch(d->fault){
 case 0: if(d->calls==1)r->format.rate=22050;else *r=(streamoo_decode_result_t){0};break;
 case 1:r->needed=12288;r->codec_status=-6;return 42;
 case 2:r->produced=cap+1;break;
 case 3:r->consumed=bytes+1;break;
 case 4:r->format.bits=24;break;
 case 5:r->format.channels=3;break;
 case 6:r->format.rate=96000;break;
 case 7:r->produced=3;break;
 case 8:if(d->calls>1)r->format.rate=22050;break;
 }
 return ESP_OK;
}
static void diagnostic(unsigned fault){
 diagnostic_t d={.fault=fault};streamoo_live_decode_t s;
 assert(streamoo_live_decode_init(&s,(streamoo_byte_source_t){&d,diagnostic_source,NULL},&d,diagnostic_decode));
 int32_t output[480],before[480];memset(output,0x5a,sizeof output);memcpy(before,output,sizeof output);
 assert(streamoo_live_decode_read(&s,output)==STREAMOO_STREAM_FAILED);assert(!memcmp(output,before,sizeof output));assert(s.failed);
 streamoo_live_failure_t expected=fault==0?STREAMOO_LIVE_DECODER_NO_PROGRESS:fault==1?STREAMOO_LIVE_DECODER_ERROR:
 (fault==2||fault==3)?STREAMOO_LIVE_DECODER_BOUNDS:(fault>=4&&fault<=7)?STREAMOO_LIVE_PCM_FORMAT:
 fault==8?STREAMOO_LIVE_FORMAT_CHANGED:(fault==9||fault==14)?STREAMOO_LIVE_SOURCE_EOF:
 fault==10?STREAMOO_LIVE_SOURCE_ERROR:STREAMOO_LIVE_SOURCE_CONTRACT;
 assert(s.failure==expected&&strcmp(streamoo_live_failure_name(s.failure),"none"));
 if(fault==0){assert(s.format.rate==22050&&s.observed.rate==22050&&s.decode_calls==2&&s.last_input==1&&!s.last_consumed&&!s.last_produced);assert(s.fetched_total==2&&s.consumed_total==1&&s.produced_total==4);}
 if(fault==1)assert(s.decoder_error==42&&s.codec_status==-6&&s.last_needed==12288);
 if(fault==8)assert(s.format.rate==44100&&s.observed.rate==22050&&s.decode_calls==2&&s.last_consumed==1&&s.last_produced==4);
 if(fault==14)assert(s.source_calls==1&&s.decode_calls==1&&s.eof);
 uint64_t calls=s.decode_calls;assert(streamoo_live_decode_read(&s,output)==STREAMOO_STREAM_FAILED&&s.decode_calls==calls&&s.failure==expected);
 assert(!strcmp(streamoo_live_failure_name(STREAMOO_LIVE_DECODER_NO_PROGRESS),"decoder_no_progress"));
 assert(!strcmp(streamoo_live_failure_name(STREAMOO_LIVE_FORMAT_CHANGED),"format_changed"));
 assert(!strcmp(streamoo_live_failure_name((streamoo_live_failure_t)99),"unknown"));
 assert(streamoo_live_decode_init(&s,(streamoo_byte_source_t){&d,diagnostic_source,NULL},&d,diagnostic_decode));
 assert(!s.failed&&s.failure==STREAMOO_LIVE_FAILURE_NONE&&!s.decode_calls&&!s.source_calls&&!s.fetched_total&&!s.last_input);
}
int main(int argc,char **argv){assert(argc==2);int mode=atoi(argv[1]);if(mode>=100)diagnostic(mode-100);else if(mode<3)resample(mode==0?48000:mode==1?44100:22050,mode==2?1:2);else icy(mode-2);return 0;}
'''
class LiveStreamTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name)
  (base/'esp_err.h').write_text('#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n')
  (base/'test.c').write_text(HARNESS);cls.binary=base/'live'
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_LIVE_SANITIZE','').split(),'-I'+str(base),'-Icomponents/streaming/include','-Icomponents/decoder/include','-Icomponents/compressed_source/include','-Icomponents/pcm_adapter/include',str(base/'test.c'),'components/streaming/streamoo_icy.c','components/streaming/streamoo_live_decode.c','-o',str(cls.binary)],cwd=ROOT,check=True)
 def test_48k_stereo_identity(self):subprocess.run([str(self.binary),'0'],check=True)
 def test_44100_fractional_phase_across_wait(self):subprocess.run([str(self.binary),'1'],check=True)
 def test_mono_22050_and_eof(self):subprocess.run([str(self.binary),'2'],check=True)
 def test_icy_arbitrary_fragmentation(self):
  for i in range(3,40):subprocess.run([str(self.binary),str(i)],check=True)

 def test_failure_reasons_snapshots_and_reset(self):
  for fault in range(15):
   with self.subTest(fault=fault):subprocess.run([str(self.binary),str(100+fault)],check=True)
