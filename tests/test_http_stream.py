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
int esp_http_client_close(void *);
int esp_http_client_read(void *,char *,int);
int esp_http_client_cleanup(void *);
'''
HARNESS=r'''
#include "streamoo_http_stream.h"
#include "esp_http_client.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
static int mode,redirected,reads,cleaned,semaphores,allocations,opened;
static void (*reader)(void *);static void *reader_context;static jmp_buf completed;
static esp_http_client_config_t config;
static streamoo_http_stream_t *stream;
static uint8_t wire[128];static size_t cursor,wire_bytes;
void *heap_caps_malloc(size_t n,unsigned caps){(void)caps;void *p=malloc(n);if(p)++allocations;return p;}
void *heap_caps_calloc(size_t n,size_t s,unsigned caps){(void)caps;void *p=calloc(n,s);if(p)++allocations;return p;}
void heap_caps_free(void *p){if(p){assert(allocations);--allocations;free(p);}}
void *xSemaphoreCreateBinary(void){int *p=calloc(1,sizeof *p);++semaphores;return p;}
void *xSemaphoreCreateMutex(void){int *p=xSemaphoreCreateBinary();*p=1;return p;}
int xSemaphoreTake(void *p,unsigned wait){(void)wait;if(!*(int *)p)return 0;*(int *)p=0;return 1;}
int xSemaphoreGive(void *p){*(int *)p=1;return 1;}
void vSemaphoreDelete(void *p){--semaphores;free(p);}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned size,void *ctx,unsigned pri,void *handle){(void)name;(void)handle;assert(size>=8192&&pri==3);reader=fn;reader_context=ctx;return mode==7?0:1;}
void vTaskDelay(unsigned t){(void)t;}
unsigned uxTaskGetStackHighWaterMark(void *task){(void)task;return 8000;}
void vTaskDelete(void *p){(void)p;longjmp(completed,1);}
int esp_crt_bundle_attach(void *p){(void)p;return ESP_OK;}
void *esp_http_client_init(const esp_http_client_config_t *c){config=*c;assert(c->timeout_ms==3000&&c->disable_auto_redirect&&c->crt_bundle_attach==esp_crt_bundle_attach&&c->auth_type==0);return (void *)1;}
int esp_http_client_set_header(void *h,const char *key,const char *value){(void)h;if(!strcmp(key,"Icy-MetaData"))assert(!strcmp(value,"1"));else assert(!strcmp(key,"Accept-Encoding")&&!strcmp(value,"identity"));return ESP_OK;}
int esp_http_client_open(void *h,int size){(void)h;assert(!size);++opened;return ESP_OK;}
static void header(const char *name,const char *value){esp_http_client_event_t e={HTTP_EVENT_ON_HEADER,config.user_data,(char *)name,(char *)value};assert(config.event_handler(&e)==ESP_OK);}
long long esp_http_client_fetch_headers(void *h){(void)h;
 if(mode==12)header("icy-br","192");
 if(mode==13){header("icy-br","-128");header("icy-br","128junk");header("icy-br","99999999999999");}
 if(mode==14&&!redirected)header("icy-br","320");
 header("Content-Type",mode==5?"text/html":mode==11?"audio/mpegjunk":mode==1?"application/octet-stream":"audio/mpeg");header("icy-metaint",mode==4?"999999999":"3");if(mode==10)header("Location","http://user:key@radio.invalid");return 0;}
int esp_http_client_get_status_code(void *h){(void)h;return (mode==2||mode==3||mode==10||mode==14)&&!redirected?302:200;}
int esp_http_client_get_url(void *h,char *out,int cap){(void)h;const char *url=mode==3?(redirected?"http://radio.invalid":"https://radio.invalid"):(redirected?"https://radio.invalid":"http://radio.invalid");assert((size_t)cap>strlen(url));strcpy(out,url);return ESP_OK;}
int esp_http_client_set_redirection(void *h){(void)h;redirected=1;return ESP_OK;}
int esp_http_client_close(void *h){(void)h;return ESP_OK;}
int esp_http_client_cleanup(void *h){assert(h);++cleaned;return ESP_OK;}
int esp_http_client_read(void *h,char *out,int cap){(void)h;++reads;if(mode==9||(mode==8&&reads<=2))return -ESP_ERR_HTTP_EAGAIN;if(mode==6)streamoo_http_stream_stop(stream);if(cursor==wire_bytes)return 0;size_t n=wire_bytes-cursor;if(n>(size_t)cap)n=cap;if(mode==1)n=1;memcpy(out,wire+cursor,n);cursor+=n;return n;}
int main(int argc,char **argv){assert(argc==2);mode=atoi(argv[1]);
 memcpy(wire,"ID3",3);wire[3]=2;memcpy(wire+4,"StreamTitle='Test';",19);memcpy(wire+36,"abc",3);wire[39]=0;memcpy(wire+40,"def",3);wire_bytes=43;
 assert(streamoo_http_stream_open("ftp://radio.invalid",&stream)==ESP_ERR_INVALID_ARG);
 int e=streamoo_http_stream_open(mode==3?"https://radio.invalid":"http://radio.invalid",&stream);
 if(mode==7){assert(e==ESP_ERR_NO_MEM&&!allocations&&!semaphores);return 0;}
 assert(e==ESP_OK&&!streamoo_http_stream_dispose(stream));
 if(!setjmp(completed))reader(reader_context);
 assert(cleaned==1);if(mode==8)assert(opened==1);
 streamoo_http_stream_info_t info;assert(streamoo_http_stream_info(stream,&info)&&info.ended);
 assert(info.bitrate_kbps==(mode==12?192u:0u));
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
  files=dict(COMMON);files['esp_err.h']+='\n#define ESP_ERR_INVALID_RESPONSE 4\n#define ESP_ERR_NOT_SUPPORTED 5\n#define ESP_ERR_TIMEOUT 6\n'
  files['esp_heap_caps.h']+='#define MALLOC_CAP_SPIRAM 4\nvoid *heap_caps_malloc(size_t,unsigned);\n'
  files['freertos/task.h']+='void vTaskDelete(void *);\n'
  files['freertos/semphr.h']='#pragma once\ntypedef void *SemaphoreHandle_t;\ntypedef unsigned TickType_t;\nvoid *xSemaphoreCreateBinary(void);\nvoid *xSemaphoreCreateMutex(void);\nint xSemaphoreTake(void *,unsigned);\nint xSemaphoreGive(void *);\nvoid vSemaphoreDelete(void *);\n'
  files['esp_http_client.h']=HTTP;files['esp_crt_bundle.h']='#pragma once\nint esp_crt_bundle_attach(void *);\n'
  for name,body in files.items():(base/name).write_text(body)
  (base/'test.c').write_text(HARNESS);cls.binary=base/'http'
  includes=['-I'+str(base)]+['-Icomponents/'+n+'/include' for n in ('streaming','decoder','pcm_adapter','compressed_source')]
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_HTTP_SANITIZE','').split(),*includes,str(base/'test.c'),'components/streaming/streamoo_http_stream.c','components/streaming/streamoo_icy.c','components/compressed_source/streamoo_buffered_source.c','components/compressed_source/streamoo_byte_source.c','-o',str(cls.binary)],cwd=ROOT,check=True)
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
