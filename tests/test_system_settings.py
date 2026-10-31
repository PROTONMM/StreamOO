"""Real NVS settings adapter and deferred station worker, synthetic SDK only."""
import os
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include "streamoo_settings.h"
#include "nvs.h"
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>
static unsigned mode,commits,station_writes,delays;static int64_t now;
static uint8_t stored[68];static uint64_t saved_id;static void (*worker)(void*);static jmp_buf end;
int nvs_flash_init(void){return 0;}
int nvs_open(const char *ns,int access,nvs_handle_t *h){(void)access;assert(!strcmp(ns,"streamoo_system"));*h=1;return 0;}
int nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n){(void)h;assert(!strcmp(key,"settings"));if(!mode)return ESP_ERR_NVS_NOT_FOUND;assert(*n==68);memcpy(out,stored,68);*n=68;return 0;}
int nvs_set_blob(nvs_handle_t h,const char *key,const void *data,size_t n){(void)h;assert(!strcmp(key,"settings")&&n==68);memcpy(stored,data,n);return 0;}
int nvs_get_u64(nvs_handle_t h,const char *key,uint64_t *out){(void)h;assert(!strcmp(key,"station"));*out=77;return 0;}
int nvs_set_u64(nvs_handle_t h,const char *key,uint64_t id){(void)h;assert(!strcmp(key,"station"));assert(now>=5000000);saved_id=id;++station_writes;return 0;}
int nvs_commit(nvs_handle_t h){(void)h;++commits;return mode==2?ESP_FAIL:0;}
void nvs_close(nvs_handle_t h){(void)h;}
void *xSemaphoreCreateMutex(void){return (void*)1;}
int xSemaphoreTake(void *s,unsigned wait){(void)s;(void)wait;return 1;}
int xSemaphoreGive(void *s){(void)s;return 1;}
int64_t esp_timer_get_time(void){return now;}
int xTaskCreate(void(*f)(void*),const char *name,unsigned stack,void *ctx,unsigned pri,void *handle){(void)ctx;(void)handle;assert(!strcmp(name,"system_settings")&&stack>=4096&&pri==1);worker=f;return 1;}
void vTaskDelay(unsigned ticks){now+=(int64_t)ticks*1000;if(++delays==26)longjmp(end,1);}
int main(int argc,char **argv){assert(argc==2);mode=(unsigned)atoi(argv[1]);stored[0]=mode==4?255:1;stored[1]=7;stored[2]=0;stored[3]=1;strcpy((char*)stored+4,"my-radio");
 assert(streamoo_settings_start()==0);assert(worker);streamoo_settings_t s=streamoo_settings_get();
 assert(s.list_timeout_s==((mode==0||mode==4)?5u:7u));assert(streamoo_settings_last_station()==77);
 if(mode==1){streamoo_settings_station_changed(88);streamoo_settings_station_changed(88);assert(!station_writes);if(!setjmp(end))worker(NULL);assert(station_writes==1&&saved_id==88&&streamoo_settings_last_station()==88);return 0;}
 s.list_timeout_s=9;strcpy(s.hostname,"new-radio");s.autoplay=true;
 assert(streamoo_settings_set(&s)==(mode==2?ESP_FAIL:0));assert(streamoo_settings_get().list_timeout_s==(mode==2?7u:9u));
 unsigned before=commits;
 for(unsigned i=0;i<4;++i){s.list_timeout_s=i==0?0:i==1?121:5;strcpy(s.hostname,i==2?"-bad":i==3?"bad.local":"okay");assert(!streamoo_settings_valid(&s));assert(streamoo_settings_set(&s)==ESP_ERR_INVALID_ARG);}
 assert(commits==before);return 0;}
'''
class SystemSettingsTests(unittest.TestCase):
 def test_persistence_validation_failure_and_debounce(self):
  with tempfile.TemporaryDirectory() as tmp:
   base=Path(tmp);(base/'freertos').mkdir()
   (base/'esp_err.h').write_text('#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_INVALID_STATE 2\n#define ESP_ERR_NO_MEM 3\n#define ESP_ERR_TIMEOUT 4\n#define ESP_ERR_NVS_NOT_FOUND 5\n')
   (base/'nvs.h').write_text('#pragma once\n#include <stddef.h>\n#include <stdint.h>\n#include "esp_err.h"\ntypedef int nvs_handle_t;\n#define NVS_READONLY 0\n#define NVS_READWRITE 1\nint nvs_open(const char*,int,nvs_handle_t*);\nint nvs_get_blob(nvs_handle_t,const char*,void*,size_t*);\nint nvs_set_blob(nvs_handle_t,const char*,const void*,size_t);\nint nvs_get_u64(nvs_handle_t,const char*,uint64_t*);\nint nvs_set_u64(nvs_handle_t,const char*,uint64_t);\nint nvs_commit(nvs_handle_t);\nvoid nvs_close(nvs_handle_t);\n')
   (base/'nvs_flash.h').write_text('int nvs_flash_init(void);\n')
   (base/'esp_timer.h').write_text('#include <stdint.h>\nint64_t esp_timer_get_time(void);\n')
   from test_network_player import HEADERS
   for name in ('freertos/FreeRTOS.h','freertos/task.h'):(base/name).write_text(HEADERS[name])
   (base/'freertos/semphr.h').write_text('#pragma once\ntypedef void *SemaphoreHandle_t;\nvoid *xSemaphoreCreateMutex(void);\nint xSemaphoreTake(void*,unsigned);\nint xSemaphoreGive(void*);\n')
   (base/'test.c').write_text(HARNESS);binary=base/'test'
   subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_SETTINGS_SANITIZE','').split(),'-I'+tmp,'-Icomponents/settings/include','components/settings/streamoo_settings.c',str(base/'test.c'),'-o',str(binary)],cwd=ROOT,check=True)
   for mode in (0,1,2,4):
    with self.subTest(mode=mode):subprocess.run([str(binary),str(mode)],check=True)
