"""Run the actual setup HTTP handlers against deterministic transport/backend fakes."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
HEADER=r'''
#pragma once
#include <stddef.h>
#include "esp_err.h"
typedef void *httpd_handle_t;
typedef struct { size_t content_len; void *user_ctx; } httpd_req_t;
typedef struct { const char *uri; int method; esp_err_t (*handler)(httpd_req_t *); void *user_ctx; } httpd_uri_t;
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTPD_RESP_USE_STRLEN -1
esp_err_t httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t *);
esp_err_t httpd_resp_set_type(httpd_req_t *,const char *);
esp_err_t httpd_resp_set_hdr(httpd_req_t *,const char *,const char *);
esp_err_t httpd_resp_set_status(httpd_req_t *,const char *);
esp_err_t httpd_resp_send(httpd_req_t *,const char *,long);
esp_err_t httpd_resp_send_chunk(httpd_req_t *,const char *,long);
size_t httpd_req_get_hdr_value_len(httpd_req_t *,const char *);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *,const char *,char *,size_t);
int httpd_req_recv(httpd_req_t *,char *,size_t);
'''
HARNESS=r'''
#include "streamoo_portal_config.h"
#include "streamoo_settings.h"
#include "streamoo_config_store.h"
#include "esp_http_server.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
esp_err_t streamoo_portal_register_configuration(httpd_handle_t);
void streamoo_portal_config_stopped(void);
static httpd_uri_t routes[14];static unsigned count,submits,previews,commits;
static bool settings_fail;
static streamoo_settings_t system_settings={.list_timeout_s=5,.list_autoselect=true};
streamoo_settings_t streamoo_settings_get(void){return system_settings;}
bool streamoo_settings_valid(const streamoo_settings_t *s){return s->list_timeout_s>=1&&s->list_timeout_s<=120;}
esp_err_t streamoo_settings_set(const streamoo_settings_t *s){if(settings_fail)return ESP_FAIL;system_settings=*s;return ESP_OK;}
static int proof=1,foreign,timeout,closed;static bool player_mode_active=true;static const char *catalog_gen="42";
static const char *input="",*type="application/json";static size_t received;
static char status[64],output[4096];static streamoo_wifi_profiles_t saved;
void *heap_caps_malloc(size_t n,unsigned caps){(void)caps;return malloc(n);}
void heap_caps_free(void *p){free(p);}
static const char *header(const char *name){
 if(!strcmp(name,"X-StreamOO-Catalog-Generation"))return catalog_gen;
 if(!strcmp(name,"X-StreamOO-Request"))return proof?"1":"";
 if(!strcmp(name,"Content-Type"))return type;
 if(!strcmp(name,"Origin"))return foreign?"http://foreign.invalid":"http://192.168.4.1";
 if(!strcmp(name,"Host"))return "192.168.4.1";return "";
}
size_t httpd_req_get_hdr_value_len(httpd_req_t *r,const char *name){(void)r;return strlen(header(name));}
int httpd_req_get_hdr_value_str(httpd_req_t *r,const char *name,char *out,size_t cap){
 (void)r;const char *s=header(name);if(!*s||strlen(s)>=cap)return ESP_FAIL;strcpy(out,s);return ESP_OK;
}
int httpd_req_recv(httpd_req_t *r,char *out,size_t cap){(void)r;if(timeout)return -1;
 size_t n=strlen(input)-received;if(n>cap)n=cap;if(n>3)n=3;memcpy(out,input+received,n);received+=n;return n;
}
int httpd_resp_set_status(httpd_req_t *r,const char *s){(void)r;strcpy(status,s);return ESP_OK;}
int httpd_resp_set_type(httpd_req_t *r,const char *s){(void)r;(void)s;return ESP_OK;}
int httpd_resp_set_hdr(httpd_req_t *r,const char *key,const char *value){(void)r;
 if(!strcmp(key,"Connection"))closed=!strcmp(value,"close");assert(strcmp(key,"Access-Control-Allow-Origin"));return ESP_OK;}
int httpd_resp_send(httpd_req_t *r,const char *s,long n){(void)r;(void)n;assert(strlen(s)<sizeof output);strcpy(output,s);return ESP_OK;}
int httpd_resp_send_chunk(httpd_req_t *r,const char *s,long n){(void)r;if(!n)return ESP_OK;assert(strlen(output)+(size_t)n<sizeof output);strncat(output,s,(size_t)n);return ESP_OK;}
int httpd_register_uri_handler(httpd_handle_t h,const httpd_uri_t *r){assert(h&&count<14);routes[count++]=*r;return ESP_OK;}
static bool load(streamoo_wifi_profiles_t *p){*p=saved;return true;}
static int submit(const streamoo_wifi_profiles_t *p){assert(p->count==1);assert(!strcmp(p->profiles[0].ssid,"New"));++submits;return ESP_OK;}
static int playlist(uint8_t *p,size_t n,bool commit,streamoo_playlist_report_t *r){
 streamoo_playlist_workspace_t *ws=calloc(1,sizeof *ws);bool ok=streamoo_playlist_validate(p,&n,r,ws);free(ws);
 if(!ok)return ESP_ERR_INVALID_ARG;if(commit)++commits;else ++previews;return ESP_OK;
}
static bool info(streamoo_playlist_report_t *r){*r=(streamoo_playlist_report_t){.rows=commits?1:0};return true;}
static void call(unsigned i,const char *body,size_t declared){input=body;received=0;closed=0;output[0]=0;strcpy(status,"200 OK");httpd_req_t r={declared,routes[i].user_ctx};
 int result=routes[i].handler(&r);assert(result==(status[0]>='4'?ESP_FAIL:ESP_OK));assert(closed);}
static const char *zone="Europe/Warsaw";
static const char *get_zone(void){return zone;}
static esp_err_t set_zone(const char *value){if(!strcmp(value,"UTC")){zone="UTC";return ESP_OK;}return ESP_FAIL;}
static unsigned controls;
static esp_err_t player_command(streamoo_portal_player_action_t action,uint8_t value){
 if(action==STREAMOO_PORTAL_PLAY)return ESP_ERR_INVALID_STATE;
 if(action==STREAMOO_PORTAL_VOLUME){assert(value==57);++controls;return ESP_OK;}
 return ESP_FAIL;
}
struct streamoo_catalog_snapshot{int refs;};static streamoo_catalog_snapshot_t snapshot;
static const streamoo_station_t stations[]={{.stable_id=11,.name=(const uint8_t *)"A",.url=(const uint8_t *)"http://a.invalid",.name_bytes=1,.url_bytes=16,.ovol=0}};
streamoo_catalog_snapshot_t *streamoo_config_catalog_acquire(void){++snapshot.refs;return &snapshot;}
streamoo_catalog_view_t streamoo_config_catalog_view(const streamoo_catalog_snapshot_t *s){assert(s);return (streamoo_catalog_view_t){stations,1,42};}
void streamoo_config_catalog_release(streamoo_catalog_snapshot_t *s){assert(s&&s->refs);--s->refs;}
static bool in_player_mode(void){return player_mode_active;}
static esp_err_t choose(uint64_t id,uint64_t gen){return id==11&&gen==42?ESP_OK:ESP_ERR_INVALID_STATE;}
static esp_err_t replace(uint8_t *p,size_t n,uint64_t gen,streamoo_playlist_report_t *r){if(gen!=42)return ESP_ERR_INVALID_STATE;if(!n){*r=(streamoo_playlist_report_t){0};return ESP_OK;}return playlist(p,n,true,r);}
esp_err_t streamoo_config_catalog_edit(uint8_t *p,size_t n,uint64_t gen,streamoo_playlist_report_t *r){return replace(p,n,gen,r);}
int main(void){
 streamoo_portal_actions_t a={load,submit,playlist,info,get_zone,set_zone,player_command,in_player_mode,choose,replace};assert(streamoo_portal_configure(&a)==ESP_OK);
 assert(streamoo_portal_register_configuration((void *)1)==ESP_OK&&count==14);assert(streamoo_portal_configure(&a)==ESP_ERR_INVALID_STATE);
 call(12,"",0);assert(strstr(status,"200")&&strstr(output,"list_timeout_s"));
 const char *settings="{\"list_timeout_s\":9,\"list_autoselect\":false,\"autoplay\":true,\"hostname\":\"kitchen-radio\"}";
 proof=0;call(13,settings,strlen(settings));assert(strstr(status,"403"));proof=1;
 call(13,settings,strlen(settings));assert(strstr(status,"200")&&system_settings.list_timeout_s==9&&system_settings.autoplay&&!system_settings.list_autoselect&&!strcmp(system_settings.hostname,"kitchen-radio"));
 settings_fail=true;call(13,settings,strlen(settings));assert(strstr(status,"503"));settings_fail=false;
 const char *bad_settings="{\"list_timeout_s\":0,\"list_autoselect\":false,\"autoplay\":true,\"hostname\":\"test\"}";
 call(13,bad_settings,strlen(bad_settings));assert(strstr(status,"400"));
 call(13,"{}",2);assert(strstr(status,"400"));
 call(5,"",0);assert(strstr(output,"Europe/Warsaw")&&strstr(output,"Europe/London"));
 proof=0;call(6,"{}",2);assert(strstr(status,"403"));proof=1;
 call(6,"{\"timezone\":\"bad\"}",18);assert(strstr(status,"400"));
 const char *utc="{\"timezone\":\"UTC\"}";call(6,utc,strlen(utc));assert(strstr(status,"200")&&!strcmp(zone,"UTC"));
 const char *warsaw="{\"timezone\":\"Europe/Warsaw\"}";call(6,warsaw,strlen(warsaw));assert(strstr(status,"503")&&!strcmp(zone,"UTC"));
 call(6,"x",129);assert(strstr(status,"413"));
 const char *volume="{\"action\":\"volume\",\"value\":57}";
 call(7,volume,strlen(volume));assert(strstr(status,"202")&&controls==1);
 const char *play="{\"action\":\"play\"}";call(7,play,strlen(play));assert(strstr(status,"409"));
 const char *stop="{\"action\":\"stop\"}";call(7,stop,strlen(stop));assert(strstr(status,"503"));
 const char *bad="{\"action\":\"volume\",\"value\":57.5}";call(7,bad,strlen(bad));assert(strstr(status,"400")&&controls==1);
 const char *range="{\"action\":\"volume\",\"value\":101}";call(7,range,strlen(range));assert(strstr(status,"400")&&controls==1);
 const char *extra="{\"action\":\"play\",\"value\":0}";call(7,extra,strlen(extra));assert(strstr(status,"400"));
 proof=0;call(7,volume,strlen(volume));assert(strstr(status,"403")&&controls==1);proof=1;
 call(8,"",0);assert(strstr(output,"000000000000000b")&&strstr(output,"http://a.invalid")&&!snapshot.refs);
 call(9,"",0);assert(!strcmp(output,"A\thttp://a.invalid\t0\n")&&!snapshot.refs);
 const char *select="{\"station_id\":\"000000000000000b\"}";
 call(11,select,strlen(select));assert(strstr(status,"202"));
 catalog_gen="41";call(11,select,strlen(select));assert(strstr(status,"409"));
 type="text/plain";call(10,"",0);assert(strstr(status,"409"));catalog_gen="42";
 call(10,"",0);assert(strstr(status,"200")&&strstr(output,"\"rows\":0"));
 type="application/json";player_mode_active=false;call(8,"",0);assert(strstr(status,"409"));
 call(9,"",0);assert(strstr(status,"409"));call(11,select,strlen(select));assert(strstr(status,"409"));
 type="text/plain";call(10,"",0);assert(strstr(status,"409"));type="application/json";
 player_mode_active=true;
 saved.count=1;strcpy(saved.profiles[0].ssid,"Saved");strcpy(saved.profiles[0].password,"never-return-this");
 call(0,"",0);assert(strstr(status,"200"));
 proof=0;call(1,"ignored",7);assert(strstr(status,"403"));proof=1;foreign=1;call(1,"ignored",7);assert(strstr(status,"403"));foreign=0;
 call(0,"",0);assert(strstr(status,"200")&&strstr(output,"Saved")&&strstr(output,"has_password")&&!strstr(output,"never-return-this"));
 const char *json="{\"profiles\":[{\"ssid\":\"New\",\"password\":\"test-only-key\"}]}";
 type="application/jsonjunk";call(1,json,strlen(json));assert(strstr(status,"415")&&submits==0);type="application/json";
 call(1,json,strlen(json));assert(strstr(status,"202")&&submits==1);
 call(1,"{}",2);assert(strstr(status,"400")&&submits==1);
 call(1,"x",4097);assert(strstr(status,"413")&&received==0);
 timeout=1;call(1,json,strlen(json));assert(strstr(status,"408")&&submits==1);timeout=0;
 type="text/plain; charset=utf-8";const char *csv="A\thttp://radio.invalid/live\t0";
 call(2,csv,strlen(csv));assert(strstr(status,"200")&&previews==1&&commits==0&&strstr(output,"\"committed\":false"));
 call(3,csv,strlen(csv));assert(strstr(status,"200")&&commits==1&&strstr(output,"\"committed\":true"));
 call(3,"bad",3);assert(strstr(status,"400")&&commits==1&&strstr(output,"error_line"));
 call(4,"",0);assert(strstr(output,"\"rows\":1"));
 for(int i=0;i<10;++i){call(0,"",0);assert(strstr(status,"200"));}
 streamoo_portal_config_stopped();assert(streamoo_portal_configure(&a)==ESP_OK);return 0;
}
'''
class PortalConfigurationTests(unittest.TestCase):
    def test_open_access_origin_validation_upload_and_failure_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp)
            (base/'esp_http_server.h').write_text(HEADER)
            (base/'esp_err.h').write_text('#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_INVALID_STATE 2\n#define ESP_ERR_NO_MEM 3\n')
            (base/'esp_heap_caps.h').write_text('#pragma once\n#include <stddef.h>\n#define MALLOC_CAP_SPIRAM 1\n#define MALLOC_CAP_8BIT 2\nvoid *heap_caps_malloc(size_t,unsigned);\nvoid heap_caps_free(void *);\n')
            (base/'test.c').write_text(HARNESS)
            subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',*os.environ.get('STREAMOO_PORTAL_CONFIG_SANITIZE','').split(),
                            '-I'+tmp,'-Icomponents/portal/include','-Icomponents/clock/include','-Icomponents/settings/include','-Icomponents/stations/include','-Icomponents/configuration/include','-Imanaged_components/espressif__cjson/cJSON',
                            str(base/'test.c'),'components/portal/streamoo_portal_config.c','components/clock/streamoo_clock.c','components/configuration/streamoo_config_model.c',
                            'managed_components/espressif__cjson/cJSON/cJSON.c','-lm','-o',str(base/'test')],cwd=ROOT,check=True)
            subprocess.run([str(base/'test')],check=True,timeout=10)
