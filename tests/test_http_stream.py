"""Actual HTTP reader + ICY + compressed buffer; SDK transport/RTOS fakes only."""
from pathlib import Path
import os,subprocess,tempfile,unittest
from test_network_player import HEADERS as COMMON
ROOT=Path(__file__).resolve().parents[1]
HTTP=r'''
#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
typedef void *esp_http_client_handle_t;
typedef struct {int event_id;void *user_data;char *header_key,*header_value;} esp_http_client_event_t;
typedef struct {const char *url;int timeout_ms,buffer_size,buffer_size_tx;bool disable_auto_redirect;esp_err_t (*event_handler)(esp_http_client_event_t *);void *user_data;esp_err_t (*crt_bundle_attach)(void *);const char *user_agent;int auth_type;} esp_http_client_config_t;
#define ESP_ERR_HTTP_EAGAIN 10
#define HTTP_AUTH_TYPE_NONE 0
#define HTTP_EVENT_ON_HEADER 1
void *esp_http_client_init(const esp_http_client_config_t *);
int esp_http_client_set_header(void *,const char *,const char *);
int esp_http_client_open(void *,int);
long long esp_http_client_fetch_headers(void *);
int esp_http_client_get_status_code(void *);
int esp_http_client_get_url(void *,char *,int);
int esp_http_client_set_redirection(void *);
int esp_http_client_set_url(void *,const char *);
int esp_http_client_close(void *);
int esp_http_client_read(void *,char *,int);
int esp_http_client_cleanup(void *);
int esp_http_client_get_errno(void *);
int esp_http_client_get_and_clear_last_tls_error(void *,int *,int *);
'''
HARNESS=r'''
#include "streamoo_http_stream.h"
#include "esp_http_client.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
static int sdk_get_urls,force_source_busy;
static int mode,redirected,reads,cleaned,semaphores,allocations,opened,resolved;
static char selected_url[2049];
static void (*reader)(void *);static void *reader_context;static jmp_buf completed;
static esp_http_client_config_t config;
static streamoo_http_stream_t *stream;
static unsigned media_loads,segments;
static uint8_t wire[4096];static size_t cursor,wire_bytes;
void *heap_caps_malloc(size_t n,unsigned caps){(void)caps;void *p=malloc(n);if(p)++allocations;return p;}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){(void)caps;void *p=calloc(n,s);if(p)++allocations;return p;}
void heap_caps_free(void *p){if(p){assert(allocations);--allocations;free(p);}}
void *xSemaphoreCreateBinary(void){int *p=calloc(1,sizeof *p);++semaphores;return p;}
void *xSemaphoreCreateMutex(void){int *p=xSemaphoreCreateBinary();*p=1;return p;}
int xSemaphoreTake(void *p,unsigned wait){(void)wait;if(force_source_busy||!*(int *)p)return 0;*(int *)p=0;return 1;}
int xSemaphoreGive(void *p){*(int *)p=1;return 1;}
void vSemaphoreDelete(void *p){--semaphores;free(p);}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned size,void *ctx,unsigned pri,void *handle){(void)name;(void)handle;assert(size>=8192&&pri==3);reader=fn;reader_context=ctx;return mode==7?0:1;}
void vTaskDelay(unsigned t){(void)t;if(mode==30&&segments)streamoo_http_stream_stop(stream);}
unsigned uxTaskGetStackHighWaterMark(void *task){(void)task;return 8000;}
void vTaskDelete(void *p){(void)p;longjmp(completed,1);}
int esp_crt_bundle_attach(void *p){(void)p;return ESP_OK;}
void *esp_http_client_init(const esp_http_client_config_t *c){config=*c;strcpy(selected_url,c->url);assert(c->timeout_ms==3000&&c->disable_auto_redirect&&c->crt_bundle_attach==esp_crt_bundle_attach&&c->auth_type==0);return (void *)1;}
int esp_http_client_set_header(void *h,const char *key,const char *value){(void)h;if(!strcmp(key,"Icy-MetaData"))assert(!strcmp(value,"1")||!strcmp(value,"0"));else assert(!strcmp(key,"Accept-Encoding")&&!strcmp(value,"identity"));return ESP_OK;}
int esp_http_client_open(void *h,int size){(void)h;assert(!size);++opened;return mode==40?ESP_FAIL:ESP_OK;}
static void header(const char *name,const char *value){esp_http_client_event_t e={HTTP_EVENT_ON_HEADER,config.user_data,(char *)name,(char *)value};assert(config.event_handler(&e)==ESP_OK);}
long long esp_http_client_fetch_headers(void *h){(void)h;
 if((mode==2||mode==3||mode==10||mode==14)&&!redirected)header("Location",mode==10?"http://user:key@radio.invalid":mode==3?"http://radio.invalid":"https://radio.invalid");
 if(mode==37&&strstr(selected_url,"?lsid=first"))header("Location","?lsid=keep%2Ftoken&x=1");
 if((mode==38||mode==39)&&strstr(selected_url,"segment1")&&!strstr(selected_url,"token="))header("Location",mode==39?"http://radio.invalid/signed/segment1.mp3?token=x":"../signed/segment1.mp3?token=x%2Fy");
 if(mode==44&&strstr(selected_url,"segment"))header("icy-br","999");
 if(mode==28&&strstr(selected_url,"segment"))header("icy-br","192");
 if(mode>=27){header("Content-Type",strstr(selected_url,"segment")?"application/octet-stream":"application/vnd.apple.mpegurl");return 0;}
 if(mode>=15){
  const char *type=mode==15?"audio/flac":mode==16||mode==17?"audio/ogg":mode==18||mode==26?"application/octet-stream":resolved&&mode!=21?"audio/mpeg":mode==20?"audio/x-scpls":"audio/x-mpegurl";
  header("Content-Type",type);return 0;
 }
 if(mode==12)header("icy-br","192");
 if(mode==13){header("icy-br","-128");header("icy-br","128junk");header("icy-br","99999999999999");}
 if(mode==14&&!redirected)header("icy-br","320");
 header("Content-Type",mode==5?"text/html":mode==11?"audio/mpegjunk":mode==1?"application/octet-stream":"audio/mpeg");header("icy-metaint",mode==4?"999999999":"3");return 0;}
int esp_http_client_get_status_code(void *h){(void)h;
 if(mode==37&&strstr(selected_url,"?lsid=first"))return 302;
 if((mode==38||mode==39)&&strstr(selected_url,"segment1")&&!strstr(selected_url,"token="))return 307;
 return (mode==2||mode==3||mode==10||mode==14)&&!redirected?302:200;
}
/* Emulate pinned IDF6.1: get_url omits query, even though the next HTTP
 * request still uses it. Production must not use this lossy getter. */
int esp_http_client_get_url(void *h,char *out,int cap){(void)h;++sdk_get_urls;assert((size_t)cap>strlen(selected_url));strcpy(out,selected_url);char *q=strchr(out,'?');if(q)*q=0;return ESP_OK;}
static void hls_wire(const char *url){
 cursor=0;const char *doc=NULL;
 if(mode==42){strcpy((char *)wire,"#EXTM3U\n/live.mp3\n");wire_bytes=strlen((char *)wire);return;}
 if((mode==37&&strstr(url,"?lsid=first"))||((mode==38||mode==39)&&strstr(url,"segment1")&&!strstr(url,"token="))){wire_bytes=0;return;}
 if(strstr(url,"segment")){
  ++segments;memset(wire,0,768);size_t offset=0;
  if(mode>=36){memcpy(wire,"ID3\4\0\0\0\0\0\x3f",10);memcpy(wire+73,"ID3\4\0\0\0\0\0\x29",10);offset=124;}
  if(mode==27||mode==32){memcpy(wire,"ID3\4\0\0\0\0\0\4",10);offset=14;if(mode==32)wire[6]=1;}
  if(mode==28){wire[0]=wire[188]=wire[376]=0x47;wire_bytes=564;}
  else {wire[offset]=0xff;wire[offset+1]=(mode>=36||(mode==35&&segments==2))?0xfb:0xf1;wire_bytes=offset+16;}
  return;
 }
 if(mode>=36){
  if(!strstr(url,"media")||!strstr(url,"lsid="))doc=mode==37?"#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=128000\nmedia.m3u8?lsid=first\n":"#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=128000\nmedia.m3u8?lsid=keep%2Ftoken&x=1\n";
  else {
   assert(strstr(url,"media.m3u8?lsid=keep%2Ftoken&x=1"));++media_loads;
   doc=media_loads==1?"#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXT-X-MEDIA-SEQUENCE:1\n#EXTINF:1,\nsegment1.mp3\n":media_loads==2?
   "#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXT-X-MEDIA-SEQUENCE:1\n#EXTINF:1,\nsegment1.mp3\n#EXTINF:1,\nsegment2.mp3\n":
   "#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXT-X-MEDIA-SEQUENCE:2\n#EXTINF:1,\nsegment2.mp3\n#EXTINF:1,\nsegment3.mp3\n#EXT-X-ENDLIST\n";
  }
 }
 else if(mode==27&&!strstr(url,"media"))doc="#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=128000,CODECS=\"mp4a.40.2\"\nmedia.m3u8\n";
 else if(mode==33)doc="#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=64000\nmaster.m3u8\n";
 else if(mode==34)doc="#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXTINF:1,\nhttp://radio.invalid/segment1.aac\n";
 else if(mode==29||mode==30){
  ++media_loads;
  doc=media_loads==1?"#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXT-X-MEDIA-SEQUENCE:1\n#EXTINF:1,\nsegment1.aac\n":
   "#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXT-X-MEDIA-SEQUENCE:1\n#EXTINF:1,\nsegment1.aac\n#EXTINF:1,\nsegment2.aac\n#EXT-X-ENDLIST\n";
 }
 else if(mode==31)doc="#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXTINF:1,\nsegment1.aac\n#EXT-X-DISCONTINUITY\n#EXTINF:1,\nsegment2.aac\n#EXT-X-ENDLIST\n";
 else doc="#EXTM3U\n#EXT-X-TARGETDURATION:1\n#EXTINF:1,\nsegment1.aac\n#EXTINF:1,\nsegment2.aac\n#EXT-X-ENDLIST\n";
 if(mode==43&&!strstr(url,"media"))doc="#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=128000,AVERAGE-BANDWIDTH=112000\nmedia.m3u8?lsid=keep%2Ftoken&x=1\n";
 strcpy((char *)wire,doc);wire_bytes=strlen(doc);
}
int esp_http_client_set_url(void *h,const char *url){(void)h;if(mode==41||mode==42)return ESP_ERR_NO_MEM;strcpy(selected_url,url);++resolved;cursor=0;
 if(mode==2||mode==3||mode==10||mode==14){redirected=1;return ESP_OK;}
 if(mode>=27)hls_wire(url);else if(mode!=21){memcpy(wire,"ID3test",7);wire_bytes=7;}
 return ESP_OK;}
int esp_http_client_set_redirection(void *h){(void)h;assert(0);return ESP_OK;}
int esp_http_client_close(void *h){(void)h;return ESP_OK;}
int esp_http_client_get_errno(void *h){assert(h&&!cleaned);return mode==40?104:0;}
int esp_http_client_get_and_clear_last_tls_error(void *h,int *code,int *flags){assert(h&&!cleaned);*code=mode==40?-0x3000:0;*flags=mode==40?8:0;return mode==40?ESP_FAIL:ESP_OK;}
int esp_http_client_cleanup(void *h){assert(h);++cleaned;return ESP_OK;}
int esp_http_client_read(void *h,char *out,int cap){(void)h;++reads;if(mode==9||(mode==8&&reads<=2))return -ESP_ERR_HTTP_EAGAIN;if(mode==6)streamoo_http_stream_stop(stream);if(mode==25&&!resolved){streamoo_http_stream_stop(stream);return -1;}
 if(mode==23&&!resolved){memset(out,'x',(size_t)cap);return cap;}
 if(cursor==wire_bytes){return 0;}
 size_t n=wire_bytes-cursor;if(n>(size_t)cap)n=cap;if(mode==1||mode==16||mode==17||mode>=27)n=1;memcpy(out,wire+cursor,n);cursor+=n;return n;}
int main(int argc,char **argv){assert(argc==2);mode=atoi(argv[1]);
 memcpy(wire,"ID3",3);wire[3]=2;memcpy(wire+4,"StreamTitle='Test';",19);memcpy(wire+36,"abc",3);wire[39]=0;memcpy(wire+40,"def",3);wire_bytes=43;
 if(mode==15||mode==18){memcpy(wire,"fLaCtest",8);wire_bytes=8;}
 if(mode==16||mode==17){memset(wire,0,sizeof wire);memcpy(wire,"OggS",4);wire[5]=2;wire[26]=1;wire[27]=19;
 memcpy(wire+28,mode==16?"\1vorbis":"OpusHead",mode==16?7:8);wire_bytes=36;}
 if(mode>=19){const char *doc=mode==20?"[playlist]\nFile1=/live.mp3\n":mode==22?"#EXTM3U\n#EXT-X-KEY:METHOD=AES-128,URI=\"key\"\n#EXT-X-TARGETDURATION:6\nchunk.ts\n":mode==24?"http://radio.invalid/live":"#EXTM3U\n/live.mp3\n";
 strcpy((char *)wire,doc);wire_bytes=strlen(doc);}
 if(mode>=27)hls_wire("https://radio.invalid/hls/master.m3u8");
 assert(streamoo_http_stream_open("ftp://radio.invalid",&stream)==ESP_ERR_INVALID_ARG);
 int e=streamoo_http_stream_open(mode>=27?"https://radio.invalid/hls/master.m3u8":mode>=19?"https://radio.invalid/list.m3u":mode==3?"https://radio.invalid":"http://radio.invalid",&stream);
 if(mode==7){assert(e==ESP_ERR_NO_MEM&&!allocations&&!semaphores);return 0;}
 assert(e==ESP_OK&&!streamoo_http_stream_dispose(stream));
 {
  streamoo_byte_source_t src=streamoo_http_stream_source(stream);uint8_t byte=77;size_t n=99;bool last=true;
  assert(src.read(src.context,&byte,1,&n,&last)==STREAMOO_SOURCE_WAIT&&!n&&!last&&byte==77);
  force_source_busy=1;
  assert(src.read(src.context,&byte,1,&n,&last)==STREAMOO_SOURCE_WAIT&&!n&&!last&&byte==77);
  force_source_busy=0;streamoo_http_stream_info_t waits;
  assert(streamoo_http_stream_info(stream,&waits)&&waits.source_empty_waits==1&&waits.source_busy_waits==1);
 }
 if(!setjmp(completed))reader(reader_context);
 assert(cleaned==1&&!sdk_get_urls);if(mode==8)assert(opened==1);
 streamoo_http_stream_info_t info;assert(streamoo_http_stream_info(stream,&info)&&info.ended);
 assert(info.bitrate_kbps==((mode==12||mode==28)?192u:mode==43?112u:mode==27||(mode>=36&&mode<=39)||mode==41||mode==44?128u:mode==33?64u:0u));
 if(mode==41||mode==42){assert(info.error==ESP_ERR_NO_MEM&&!strcmp(info.reason,"set_url_failed")&&!strcmp(info.stage,"set_url")&&!strcmp(info.resource,mode==41?"hls_manifest":"playlist"));streamoo_http_stream_stop(stream);assert(streamoo_http_stream_dispose(stream)&&!semaphores&&!allocations);return 0;}
 if(mode==40){assert(info.ended&&info.error==ESP_FAIL&&!strcmp(info.stage,"open")&&!strcmp(info.reason,"http_open")&&info.http_status==0&&info.socket_errno==104&&info.tls_code==-0x3000&&info.tls_flags==8&&info.tls_error==ESP_FAIL);streamoo_http_stream_stop(stream);assert(streamoo_http_stream_dispose(stream)&&!semaphores&&!allocations);return 0;}
 if(mode>=36){
 if(mode==39)assert(!segments&&!streamoo_http_stream_queued(stream)&&info.error==ESP_ERR_INVALID_RESPONSE);
 else assert(segments==3&&media_loads==3&&info.codec_known&&info.codec==STREAMOO_CODEC_MP3&&streamoo_http_stream_queued(stream)==48&&info.hls_segments==3&&info.hls_reloads==2&&info.hls_next==4&&info.hls_media_sequence==2&&info.hls_media_count==2&&!strcmp(info.reason,"endlist")&&info.reader_stack_hwm==8000);
 streamoo_http_stream_stop(stream);assert(streamoo_http_stream_dispose(stream)&&!semaphores&&!allocations);return 0;
 }
 if(mode>=27){
 if(mode==27||mode==28||mode==29){assert(segments==2);assert(info.codec_known&&info.codec==(mode==28?STREAMOO_CODEC_MPEG_TS:STREAMOO_CODEC_AAC_LC));assert(streamoo_http_stream_queued(stream)==(mode==28?1128u:32u));if(mode==29)assert(media_loads==2);}
 else if(mode==30){assert(segments==1&&media_loads==1);}
 else if(mode==31){assert(segments==1&&info.error==ESP_ERR_NOT_SUPPORTED&&!strcmp(info.reason,"discontinuity")&&info.hls_sequence==1);}
 else if(mode==32){assert(!streamoo_http_stream_queued(stream)&&info.error==ESP_ERR_INVALID_SIZE);}
 else if(mode==33){assert(!segments&&resolved==3&&info.error==ESP_ERR_NOT_SUPPORTED);}
 else if(mode==34){assert(!segments&&info.error==ESP_ERR_INVALID_RESPONSE);}
 else if(mode==35){assert(segments==2&&info.error==ESP_ERR_NOT_SUPPORTED&&streamoo_http_stream_queued(stream)==16&&!strcmp(info.reason,"codec_change"));}
 streamoo_http_stream_stop(stream);assert(streamoo_http_stream_dispose(stream));assert(!semaphores&&!allocations);return 0;
 }
 if(mode>=15){
 if(mode==15||mode==18)assert(info.codec_known&&info.codec==STREAMOO_CODEC_FLAC);
 else if(mode==16)assert(info.codec_known&&info.codec==STREAMOO_CODEC_VORBIS);
 else if(mode==17)assert(info.codec_known&&info.codec==STREAMOO_CODEC_OPUS);
 else if(mode==19||mode==20||mode==26){assert(resolved==1&&info.codec_known&&info.codec==STREAMOO_CODEC_MP3&&!strcmp(selected_url,"https://radio.invalid/live.mp3"));assert(streamoo_http_stream_queued(stream)==7);}
 else {assert(!info.codec_known&&!streamoo_http_stream_queued(stream));if(mode==21)assert(resolved==3&&info.error==ESP_ERR_NOT_SUPPORTED);if(mode==22)assert(info.error==ESP_ERR_NOT_SUPPORTED);}
 streamoo_http_stream_stop(stream);assert(streamoo_http_stream_dispose(stream));assert(!semaphores&&!allocations);return 0;
 }
 if(mode<=2||mode==8||mode>=12){assert(info.codec_known&&info.codec==STREAMOO_CODEC_MP3&&!info.title[0]);
 streamoo_byte_source_t source=streamoo_http_stream_source(stream);assert(!source.rewind);uint8_t audio[16];size_t n;bool last;
 assert(source.read(source.context,audio,2,&n,&last)==STREAMOO_SOURCE_DATA&&n==2&&!memcmp(audio,"ID",2));
 assert(streamoo_http_stream_info(stream,&info)&&!info.title[0]);
 assert(source.read(source.context,audio,sizeof audio,&n,&last)==STREAMOO_SOURCE_DATA&&n==7&&!memcmp(audio,"3abcdef",7));
 assert(streamoo_http_stream_info(stream,&info)&&!strcmp(info.title,"Test"));
 }else {assert(streamoo_http_stream_queued(stream)==0);if(mode!=6&&mode!=9)assert(reads==0);if(mode==9)assert(reads==5&&info.error==ESP_ERR_TIMEOUT);}
 streamoo_http_stream_stop(stream);assert(streamoo_http_stream_dispose(stream));assert(!semaphores&&!allocations);return 0;}
'''
class HTTPStreamTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name);(base/'freertos').mkdir()
  files=dict(COMMON);files['esp_err.h']+='\n#define ESP_ERR_INVALID_RESPONSE 4\n#define ESP_ERR_NOT_SUPPORTED 5\n#define ESP_ERR_TIMEOUT 6\n#define ESP_ERR_INVALID_SIZE 7\n'
  files['esp_heap_caps.h']+='#define MALLOC_CAP_SPIRAM 4\nvoid *heap_caps_malloc(size_t,unsigned);\n'
  files['freertos/task.h']+='void vTaskDelete(void *);\n'
  files['freertos/semphr.h']='#pragma once\ntypedef void *SemaphoreHandle_t;\ntypedef unsigned TickType_t;\nvoid *xSemaphoreCreateBinary(void);\nvoid *xSemaphoreCreateMutex(void);\nint xSemaphoreTake(void *,unsigned);\nint xSemaphoreGive(void *);\nvoid vSemaphoreDelete(void *);\n'
  files['esp_http_client.h']=HTTP;files['esp_crt_bundle.h']='#pragma once\nint esp_crt_bundle_attach(void *);\n'
  for name,body in files.items():(base/name).write_text(body)
  (base/'test.c').write_text(HARNESS);cls.binary=base/'http'
  includes=['-I'+str(base)]+['-Icomponents/'+n+'/include' for n in ('streaming','decoder','pcm_adapter','compressed_source')]
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_HTTP_SANITIZE','').split(),*includes,str(base/'test.c'),'components/streaming/streamoo_http_stream.c','components/streaming/streamoo_playlist_resolver.c','components/streaming/streamoo_hls.c','components/streaming/streamoo_icy.c','components/compressed_source/streamoo_buffered_source.c','components/compressed_source/streamoo_byte_source.c','-o',str(cls.binary)],cwd=ROOT,check=True)
 def run_case(self,n):subprocess.run([str(self.binary),str(n)],check=True,timeout=10)
 def test_http_icy_and_metadata_position(self):self.run_case(0)
 def test_octet_stream_probe_fragmented_bytewise(self):self.run_case(1)
 def test_redirect_upgrade_and_tls_configuration(self):self.run_case(2)
 def test_reject_tls_downgrade(self):self.run_case(3)
 def test_reject_invalid_meta_interval(self):self.run_case(4)
 def test_reject_html(self):self.run_case(5)
 def test_cancel_ack_before_free(self):self.run_case(6)
 def test_task_failure_cleanup(self):self.run_case(7)

 def test_temporary_timeout_keeps_same_http_session(self):self.run_case(8)
 def test_persistent_timeout_is_bounded(self):self.run_case(9)

 def test_reject_credentials_in_redirect(self):self.run_case(10)
 def test_mime_token_boundary(self):self.run_case(11)

 def test_declared_icy_bitrate(self):self.run_case(12)
 def test_invalid_optional_bitrate_does_not_break_audio(self):self.run_case(13)
 def test_redirect_bitrate_does_not_leak_to_final_stream(self):self.run_case(14)

 def test_new_formats_and_fragmented_ogg_headers(self):
  for mode in range(15,19):
   with self.subTest(mode=mode):self.run_case(mode)
 def test_m3u_pls_resolution_and_bounded_failures(self):
  for mode in range(19,27):
   with self.subTest(mode=mode):self.run_case(mode)

 def test_hls_master_packed_id3_ts_live_reload_cancel_and_rejections(self):
  for mode in range(27,36):
   with self.subTest(mode=mode):self.run_case(mode)

 def test_hls_preserves_session_query_on_multiple_reloads_and_redirects(self):
  for mode in (36,37,38):
   with self.subTest(mode=mode):self.run_case(mode)
 def test_hls_redirect_rejects_https_downgrade_with_query(self):self.run_case(39)

 def test_connection_failure_preserves_errno_tls_and_resource_stage(self):self.run_case(40)

 def test_sdk_set_url_error_is_not_overwritten_for_hls_or_m3u(self):
  for mode in (41,42):
   with self.subTest(mode=mode):self.run_case(mode)

 def test_hls_declared_bitrate_persists_across_reloads_and_segment_headers(self):
  for mode in (43,44):
   with self.subTest(mode=mode):self.run_case(mode)
