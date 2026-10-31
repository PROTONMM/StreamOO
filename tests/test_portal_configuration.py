"""Run the actual setup HTTP handlers against deterministic transport/backend fakes."""
from pathlib import Path
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
size_t httpd_req_get_hdr_value_len(httpd_req_t *,const char *);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *,const char *,char *,size_t);
int httpd_req_recv(httpd_req_t *,char *,size_t);
'''
HARNESS=r'''
#include "streamoo_portal_config.h"
#include "esp_http_server.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
esp_err_t streamoo_portal_register_configuration(httpd_handle_t);
void streamoo_portal_config_stopped(void);
static httpd_uri_t routes[5];static unsigned count,submits,previews,commits;
static int auth=1,proof=1,foreign,timeout,closed;static long long now;
static const char *input="",*type="application/json";static size_t received;
static char status[64],output[4096];static streamoo_wifi_profiles_t saved;
int64_t esp_timer_get_time(void){return now;}
void *heap_caps_malloc(size_t n,unsigned caps){(void)caps;return malloc(n);}
void heap_caps_free(void *p){free(p);}
int mbedtls_base64_decode(unsigned char *out,size_t cap,size_t *n,const unsigned char *in,size_t len){
 (void)in;(void)len;assert(cap>=25);memcpy(out,"setup:test-only-key",19);*n=19;return 0;
}
static const char *header(const char *name){
 if(!strcmp(name,"Authorization"))return auth?"Basic dGVzdA==":"";
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
int httpd_register_uri_handler(httpd_handle_t h,const httpd_uri_t *r){assert(h&&count<5);routes[count++]=*r;return ESP_OK;}
static bool authorize(const uint8_t *p,size_t n){return n==13&&!memcmp(p,"test-only-key",13);}
static bool load(streamoo_wifi_profiles_t *p){*p=saved;return true;}
static int submit(const streamoo_wifi_profiles_t *p){assert(p->count==1);assert(!strcmp(p->profiles[0].ssid,"New"));++submits;return ESP_OK;}
static int playlist(uint8_t *p,size_t n,bool commit,streamoo_playlist_report_t *r){
 streamoo_playlist_workspace_t *ws=calloc(1,sizeof *ws);bool ok=streamoo_playlist_validate(p,&n,r,ws);free(ws);
 if(!ok)return ESP_ERR_INVALID_ARG;if(commit)++commits;else ++previews;return ESP_OK;
}
static bool info(streamoo_playlist_report_t *r){*r=(streamoo_playlist_report_t){.rows=commits?1:0};return true;}
static void call(unsigned i,const char *body,size_t declared){input=body;received=0;closed=0;httpd_req_t r={declared,routes[i].user_ctx};
 int result=routes[i].handler(&r);assert(result==(status[0]>='4'?ESP_FAIL:ESP_OK));assert(closed);}
int main(void){
 streamoo_portal_actions_t a={authorize,load,submit,playlist,info};assert(streamoo_portal_configure(&a)==ESP_OK);
 assert(streamoo_portal_register_configuration((void *)1)==ESP_OK&&count==5);assert(streamoo_portal_configure(&a)==ESP_ERR_INVALID_STATE);
 saved.count=1;strcpy(saved.profiles[0].ssid,"Saved");strcpy(saved.profiles[0].password,"never-return-this");
 auth=0;call(1,"ignored",7);assert(strstr(status,"401")&&!submits&&received==0);auth=1;
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
 auth=0;for(int i=0;i<7;++i)call(0,"",0);call(0,"",0);assert(strstr(status,"429"));
 now=60000001;auth=1;call(0,"",0);assert(strstr(status,"200"));
 streamoo_portal_config_stopped();assert(streamoo_portal_configure(&a)==ESP_OK);return 0;
}
'''
class PortalConfigurationTests(unittest.TestCase):
    def test_authorization_validation_upload_and_failure_boundaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            base=Path(tmp);(base/'mbedtls').mkdir()
            (base/'esp_http_server.h').write_text(HEADER)
            (base/'esp_err.h').write_text('#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_INVALID_STATE 2\n')
            (base/'esp_heap_caps.h').write_text('#pragma once\n#include <stddef.h>\n#define MALLOC_CAP_SPIRAM 1\n#define MALLOC_CAP_8BIT 2\nvoid *heap_caps_malloc(size_t,unsigned);\nvoid heap_caps_free(void *);\n')
            (base/'esp_timer.h').write_text('#pragma once\n#include <stdint.h>\nint64_t esp_timer_get_time(void);\n')
            (base/'mbedtls/base64.h').write_text('#pragma once\n#include <stddef.h>\nint mbedtls_base64_decode(unsigned char *,size_t,size_t *,const unsigned char *,size_t);\n')
            (base/'test.c').write_text(HARNESS)
            subprocess.run(['cc','-std=c11','-D_POSIX_C_SOURCE=200809L','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                            '-I'+tmp,'-Icomponents/portal/include','-Icomponents/configuration/include','-Imanaged_components/espressif__cjson/cJSON',
                            str(base/'test.c'),'components/portal/streamoo_portal_config.c','components/configuration/streamoo_config_model.c',
                            'managed_components/espressif__cjson/cJSON/cJSON.c','-lm','-o',str(base/'test')],cwd=ROOT,check=True)
            subprocess.run([str(base/'test')],check=True,timeout=10)
