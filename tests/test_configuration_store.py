"""Real store logic with fake NVS/partition and host-backed file IO fault injection."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
HARNESS=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_partition.h"
#include "nvs.h"
static const char *directory;static int mount_result,phase,blank=1,formats,rename_error,commit_error;
static int allocation_failure;
static unsigned char blob[1024];static size_t blob_bytes;
static FILE *mapped_fopen(const char *path,const char *mode){char p[1024];snprintf(p,sizeof p,"%s/%s",directory,strrchr(path,'/')+1);return fopen(p,mode);}
static int mapped_rename(const char *from,const char *to){if(rename_error)return -1;char a[1024],b[1024];snprintf(a,sizeof a,"%s/%s",directory,strrchr(from,'/')+1);snprintf(b,sizeof b,"%s/%s",directory,strrchr(to,'/')+1);return rename(a,b);}
static int mapped_unlink(const char *path){char p[1024];snprintf(p,sizeof p,"%s/%s",directory,strrchr(path,'/')+1);return remove(p);}
#define fopen mapped_fopen
#define rename mapped_rename
#define unlink mapped_unlink
#include "streamoo_config_store.c"
#undef fopen
#undef rename
#undef unlink
void *xSemaphoreCreateMutex(void){return (void *)1;}
int xSemaphoreTake(void *p,unsigned t){(void)p;(void)t;return 1;}
int xSemaphoreGive(void *p){(void)p;return 1;}
int nvs_flash_init(void){return ESP_OK;}
int nvs_open(const char *ns,int mode,nvs_handle_t *h){(void)mode;*h=!strcmp(ns,"sfs_00f00000")?1:!strcmp(ns,"streamoo_fs")?3:2;return ESP_OK;}
void nvs_close(nvs_handle_t h){(void)h;}
int nvs_get_u8(nvs_handle_t h,const char *key,uint8_t *out){(void)key;if(h==3){*out=2;return ESP_OK;}if(!phase)return ESP_ERR_NVS_NOT_FOUND;*out=phase;return ESP_OK;}
int nvs_set_u8(nvs_handle_t h,const char *key,uint8_t value){(void)h;(void)key;phase=value;return ESP_OK;}
int nvs_commit(nvs_handle_t h){(void)h;return commit_error?ESP_FAIL:ESP_OK;}
int nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *n){(void)h;(void)key;if(!blob_bytes)return ESP_ERR_NVS_NOT_FOUND;assert(*n>=blob_bytes);memcpy(out,blob,blob_bytes);*n=blob_bytes;return ESP_OK;}
int nvs_set_blob(nvs_handle_t h,const char *key,const void *p,size_t n){(void)h;(void)key;assert(n<=sizeof blob);memcpy(blob,p,n);blob_bytes=n;return ESP_OK;}
int esp_vfs_littlefs_register(const esp_vfs_littlefs_conf_t *c){assert(!c->format_if_mount_failed);return mount_result;}
int esp_littlefs_format(const char *label){assert(!strcmp(label,"storage"));++formats;mount_result=ESP_OK;return ESP_OK;}
const esp_partition_t *esp_partition_find_first(int t,int s,const char *label){(void)t;(void)s;(void)label;static esp_partition_t p={.size=2048,.address=0xf00000};return &p;}
int esp_partition_read(const esp_partition_t *p,size_t offset,void *out,size_t n){(void)p;(void)offset;memset(out,blank?255:0,n);return ESP_OK;}
void *heap_caps_malloc(size_t n,unsigned c){(void)c;if(allocation_failure&&!--allocation_failure)return NULL;return malloc(n);}
void *heap_caps_calloc(size_t n,size_t s,unsigned c){(void)c;if(allocation_failure&&!--allocation_failure)return NULL;return calloc(n,s);}
void heap_caps_free(void *p){free(p);}
int main(int argc,char **argv){assert(argc==3);int mode=atoi(argv[1]);directory=argv[2];
 if(mode){mount_result=ESP_FAIL;if(mode==2)blank=0;if(mode==3){blank=0;phase=1;}if(mode==4){blank=0;phase=2;}if(mode==5)commit_error=1;}
 int result=streamoo_config_store_init();
 if(mode){if(mode==2||mode==4||mode==5){assert(result!=ESP_OK&&formats==0);}else assert(result==ESP_OK&&formats==1&&phase==2);return 0;}
 assert(result==ESP_OK&&formats==0&&phase==2);
 streamoo_wifi_profiles_t p={.count=1,.profiles={{"Synthetic","test-only-key"}}},q={0};assert(streamoo_config_wifi_save(&p)==ESP_OK);assert(streamoo_config_wifi_load(&q));assert(!memcmp(&p,&q,sizeof p));
 streamoo_playlist_report_t r;unsigned char old[]="Old\thttp://old.invalid\t0";
 assert(streamoo_config_playlist(old,sizeof old-1,true,&r)==ESP_OK&&r.rows==1);
 unsigned char next[]="New\thttp://new.invalid\t0\nOther\thttp://other.invalid\t0";
 assert(streamoo_config_playlist(next,sizeof next-1,false,&r)==ESP_OK&&r.rows==2);
 assert(streamoo_config_playlist_info(&r)&&r.rows==1);
 streamoo_catalog_snapshot_t *held=streamoo_config_catalog_acquire();streamoo_catalog_view_t v=streamoo_config_catalog_view(held);
 assert(v.count==1&&!strcmp((const char *)v.stations[0].name,"Old"));
 uint64_t old_id=v.stations[0].stable_id;
 for(int failure=2;failure<=4;++failure){allocation_failure=failure;assert(streamoo_config_playlist(next,sizeof next-1,true,&r)==ESP_ERR_NO_MEM);assert(streamoo_config_playlist_info(&r)&&r.rows==1);streamoo_catalog_snapshot_t *probe=streamoo_config_catalog_acquire();assert(probe==held);streamoo_config_catalog_release(probe);}
 rename_error=1;assert(streamoo_config_playlist(next,sizeof next-1,true,&r)!=ESP_OK);assert(streamoo_config_playlist_info(&r)&&r.rows==1);
 streamoo_catalog_snapshot_t *probe=streamoo_config_catalog_acquire();assert(probe==held);streamoo_config_catalog_release(probe);
 FILE *f=mapped_fopen("/stations/playlist.csv","rb");assert(f);char check[128]={0};assert(fread(check,1,sizeof check,f)>sizeof old-1);fclose(f);assert(!memcmp(check,"SOOCAT2\n",8)&&strstr(check,(char *)old));
 f=mapped_fopen("/stations/playlist.tmp","rb");assert(!f);
 rename_error=0;assert(streamoo_config_playlist(next,sizeof next-1,true,&r)==ESP_OK);assert(streamoo_config_playlist_info(&r)&&r.rows==3);
 assert(v.count==1&&!strcmp((const char *)v.stations[0].name,"Old"));
 streamoo_catalog_snapshot_t *fresh=streamoo_config_catalog_acquire();streamoo_catalog_view_t nv=streamoo_config_catalog_view(fresh);
 assert(nv.count==3&&nv.generation>v.generation&&nv.stations[0].stable_id==old_id);uint64_t id=nv.stations[1].stable_id;
 assert(streamoo_config_catalog_replace(next,sizeof next-1,v.generation,&r)==ESP_ERR_INVALID_STATE);
 assert(streamoo_config_catalog_replace(next,sizeof next-1,nv.generation,&r)==ESP_OK);
 streamoo_config_catalog_release(held);streamoo_config_catalog_release(fresh);
 mutex=NULL;mounted=false;catalog=(streamoo_playlist_report_t){0};assert(streamoo_config_store_init()==ESP_OK);assert(streamoo_config_playlist_info(&r)&&r.rows==2);
 fresh=streamoo_config_catalog_acquire();nv=streamoo_config_catalog_view(fresh);assert(nv.count==2&&nv.stations[0].stable_id==id);streamoo_config_catalog_release(fresh);
 unsigned char edited[256];int n=snprintf((char *)edited,sizeof edited,"%016llx\tRenamed\thttps://changed.invalid/live\t3\n0000000000000000\tAdded\thttp://added.invalid\t0\n",(unsigned long long)id);
 assert(streamoo_config_catalog_edit(edited,(size_t)n,nv.generation,&r)==ESP_OK);
 fresh=streamoo_config_catalog_acquire();nv=streamoo_config_catalog_view(fresh);
 assert(nv.count==2&&nv.stations[0].stable_id==id&&!strcmp((const char *)nv.stations[0].name,"Renamed"));
 uint64_t added=nv.stations[1].stable_id;assert(added&&added!=id);streamoo_config_catalog_release(fresh);
 unsigned char bad[]="ffffffffffffffff\tUnknown\thttp://a.invalid\t0\n";
 assert(streamoo_config_catalog_edit(bad,sizeof bad-1,nv.generation,&r)==ESP_ERR_INVALID_ARG);
 n=snprintf((char *)edited,sizeof edited,"%016llx\tFirst\thttp://a.invalid\t0\n%016llx\tDuplicate\thttp://b.invalid\t0\n",(unsigned long long)id,(unsigned long long)id);
 assert(streamoo_config_catalog_edit(edited,(size_t)n,nv.generation,&r)==ESP_ERR_INVALID_ARG);
 mutex=NULL;mounted=false;catalog=(streamoo_playlist_report_t){0};assert(streamoo_config_store_init()==ESP_OK);
 fresh=streamoo_config_catalog_acquire();nv=streamoo_config_catalog_view(fresh);assert(nv.stations[0].stable_id==id&&nv.stations[1].stable_id==added);streamoo_config_catalog_release(fresh);
 assert(streamoo_config_catalog_replace(NULL,0,nv.generation,&r)==ESP_OK&&r.rows==0);
 fresh=streamoo_config_catalog_acquire();nv=streamoo_config_catalog_view(fresh);assert(!nv.count);
 streamoo_config_catalog_release(fresh);
 mutex=NULL;mounted=false;catalog=(streamoo_playlist_report_t){0};assert(streamoo_config_store_init()==ESP_OK);
 fresh=streamoo_config_catalog_acquire();assert(!streamoo_config_catalog_view(fresh).count);streamoo_config_catalog_release(fresh);return 0;}
'''
class ConfigurationStoreTests(unittest.TestCase):
    def test_atomic_replace_preview_restart_and_format_guards(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);(base/'freertos').mkdir()
            files={
                'esp_err.h':'#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_INVALID_STATE 2\n#define ESP_ERR_NO_MEM 3\n#define ESP_ERR_TIMEOUT 4\n#define ESP_ERR_NVS_NOT_FOUND 5\n',
                'freertos/FreeRTOS.h':'#pragma once\n#define pdTRUE 1\ntypedef int portMUX_TYPE;\n#define portMUX_INITIALIZER_UNLOCKED 0\n#define portENTER_CRITICAL(p) ((void)(p))\n#define portEXIT_CRITICAL(p) ((void)(p))\n#define pdMS_TO_TICKS(x) (x)\n',
                'freertos/semphr.h':'#pragma once\ntypedef void *SemaphoreHandle_t;\nvoid *xSemaphoreCreateMutex(void);\nint xSemaphoreTake(void *,unsigned);\nint xSemaphoreGive(void *);\n',
                'nvs.h':'#pragma once\n#include <stdint.h>\n#include <stddef.h>\n#include "esp_err.h"\ntypedef int nvs_handle_t;\n#define NVS_READONLY 0\n#define NVS_READWRITE 1\nint nvs_open(const char *,int,nvs_handle_t *);\nvoid nvs_close(nvs_handle_t);\nint nvs_get_u8(nvs_handle_t,const char *,uint8_t *);\nint nvs_set_u8(nvs_handle_t,const char *,uint8_t);\nint nvs_commit(nvs_handle_t);\nint nvs_get_blob(nvs_handle_t,const char *,void *,size_t *);\nint nvs_set_blob(nvs_handle_t,const char *,const void *,size_t);\n',
                'nvs_flash.h':'#pragma once\nint nvs_flash_init(void);\n',
                'esp_littlefs.h':'#pragma once\n#include <stdbool.h>\ntypedef struct {const char *base_path,*partition_label;bool format_if_mount_failed;} esp_vfs_littlefs_conf_t;\nint esp_vfs_littlefs_register(const esp_vfs_littlefs_conf_t *);\nint esp_littlefs_format(const char *);\n',
                'esp_partition.h':'#pragma once\n#include <stddef.h>\ntypedef struct {size_t size;unsigned address;} esp_partition_t;\n#define ESP_PARTITION_TYPE_DATA 1\n#define ESP_PARTITION_SUBTYPE_ANY 255\nconst esp_partition_t *esp_partition_find_first(int,int,const char *);\nint esp_partition_read(const esp_partition_t *,size_t,void *,size_t);\n',
                'esp_heap_caps.h':'#pragma once\n#include <stddef.h>\n#define MALLOC_CAP_SPIRAM 1\n#define MALLOC_CAP_8BIT 2\nvoid *heap_caps_malloc(size_t,unsigned);\nvoid *heap_caps_calloc(size_t,size_t,unsigned);\nvoid heap_caps_free(void *);\n'}
            for name,body in files.items():(base/name).write_text(body)
            (base/'test.c').write_text(HARNESS)
            subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_STORE_SANITIZE','').split(),
                            '-I'+tmp,'-Icomponents/configuration','-Icomponents/configuration/include','-Icomponents/stations/include','-Imanaged_components/espressif__cjson/cJSON',
                            str(base/'test.c'),'components/configuration/streamoo_config_model.c','components/configuration/streamoo_catalog_format.c','components/stations/streamoo_stations.c','managed_components/espressif__cjson/cJSON/cJSON.c',
                            '-lm','-o',str(base/'test')],cwd=ROOT,check=True)
            for mode in range(6):
                with self.subTest(mode=mode),tempfile.TemporaryDirectory() as storage:
                    subprocess.run([str(base/'test'),str(mode),storage],check=True,timeout=10)
