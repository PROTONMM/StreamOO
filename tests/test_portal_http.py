from pathlib import Path
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
HEADER=r'''
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"
typedef void *httpd_handle_t;
typedef struct { int unused; } httpd_req_t;
typedef struct { const char *uri; int method; esp_err_t (*handler)(httpd_req_t *); void *user_ctx; } httpd_uri_t;
typedef struct { int task_priority,stack_size,max_open_sockets,max_uri_handlers; bool lru_purge_enable; int recv_wait_timeout,send_wait_timeout; } httpd_config_t;
#define HTTPD_DEFAULT_CONFIG() {0}
#define HTTP_GET 0
#define HTTPD_RESP_USE_STRLEN -1
esp_err_t httpd_start(httpd_handle_t *,const httpd_config_t *);
esp_err_t httpd_stop(httpd_handle_t);
esp_err_t httpd_register_uri_handler(httpd_handle_t,const httpd_uri_t *);
esp_err_t httpd_resp_set_type(httpd_req_t *,const char *);
esp_err_t httpd_resp_set_hdr(httpd_req_t *,const char *,const char *);
esp_err_t httpd_resp_set_status(httpd_req_t *,const char *);
esp_err_t httpd_resp_send(httpd_req_t *,const char *,long);
'''
HARNESS=r'''
#include "streamoo_portal.h"
#include "esp_http_server.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
esp_err_t streamoo_portal_register_configuration(httpd_handle_t h) {(void)h;return ESP_OK;}
void streamoo_portal_config_stopped(void) {}
static httpd_uri_t routes[4]; static int count,stops,fail_register,fail_stop,unavailable;
static char type[80],body[1024],response_status[80]; static bool no_store,csp,nosniff;
esp_err_t httpd_start(httpd_handle_t *h,const httpd_config_t *c) {
    assert(c->task_priority==2 && c->stack_size==8192 && c->max_open_sockets==4);
    assert(c->lru_purge_enable && c->recv_wait_timeout==3 && c->send_wait_timeout==3);
    count=0;*h=(void *)1; return ESP_OK;
}
esp_err_t httpd_stop(httpd_handle_t h) { assert(h); ++stops; return fail_stop ? ESP_FAIL:ESP_OK; }
esp_err_t httpd_register_uri_handler(httpd_handle_t h,const httpd_uri_t *r) {
    assert(h && count<4); if (fail_register && count==2) return ESP_FAIL;
    assert(r->method==HTTP_GET); routes[count++]=*r;return ESP_OK;
}
esp_err_t httpd_resp_set_type(httpd_req_t *r,const char *s) { (void)r;strcpy(type,s);return ESP_OK; }
esp_err_t httpd_resp_set_hdr(httpd_req_t *r,const char *key,const char *value) {
    (void)r;if (!strcmp(key,"Cache-Control")) no_store=!strcmp(value,"no-store");
    if (!strcmp(key,"Content-Security-Policy")) csp=strstr(value,"form-action 'self'")!=NULL;
    if (!strcmp(key,"X-Content-Type-Options")) nosniff=!strcmp(value,"nosniff");
    assert(strcmp(key,"Access-Control-Allow-Origin")); return ESP_OK;
}
esp_err_t httpd_resp_set_status(httpd_req_t *r,const char *s) { (void)r;strcpy(response_status,s);return ESP_OK; }
esp_err_t httpd_resp_send(httpd_req_t *r,const char *s,long n) {
    (void)r;size_t bytes=n<0 ? strlen(s):(size_t)n; assert(bytes<sizeof body);
    memcpy(body,s,bytes);body[bytes]=0;return ESP_OK;
}
static bool snapshot(void *context,streamoo_portal_status_t *s) {
    assert(context==(void *)42); if (unavailable) return false;
    *s=(streamoo_portal_status_t){.ap_ssid="StreamOO-123ABC",.clients=1,.volume=42,.ap_ready=true,.audio_available=true};return true;
}
int main(int argc,char **argv) {
    assert(argc==2); int mode=atoi(argv[1]);
    assert(streamoo_portal_start(NULL,NULL)==ESP_ERR_INVALID_ARG);
    if (mode==2) fail_register=1;
    esp_err_t result=streamoo_portal_start(snapshot,(void *)42);
    if (mode==2) {
        assert(result==ESP_FAIL && stops==1);fail_register=0;
        assert(streamoo_portal_start(snapshot,(void *)42)==ESP_OK);
    } else assert(result==ESP_OK);
    assert(count==4 && streamoo_portal_start(snapshot,(void *)42)==ESP_ERR_INVALID_STATE);
    assert(!strcmp(routes[0].uri,"/") && !strcmp(routes[3].uri,"/api/v1/status"));
    httpd_req_t request={0};
    if (mode==1) unavailable=1;
    assert(routes[3].handler(&request)==ESP_OK);
    assert(no_store && csp && nosniff && strstr(type,"application/json"));
    if (mode==1) assert(!strcmp(response_status,"503 Service Unavailable") && strstr(body,"status_unavailable"));
    else assert(strstr(body,"\"volume\":42") && !strstr(body,"password"));
    for (int i=0;i<3;++i) assert(routes[i].handler(&request)==ESP_OK);
    if (mode==3) {
        fail_stop=1;assert(streamoo_portal_stop()==ESP_FAIL);
        assert(streamoo_portal_start(snapshot,NULL)==ESP_ERR_INVALID_STATE);fail_stop=0;
    }
    assert(streamoo_portal_stop()==ESP_OK);assert(streamoo_portal_stop()==ESP_OK);
    return 0;
}
'''
class PortalHTTPTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.temp.cleanup)
        base=Path(cls.temp.name)
        (base/'esp_err.h').write_text('#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_ARG 1\n#define ESP_ERR_INVALID_STATE 2\nstatic inline const char *esp_err_to_name(int e) {(void)e;return "error";}\n')
        (base/'esp_log.h').write_text('#pragma once\n#define ESP_LOGI(...) ((void)0)\n#define ESP_LOGE(...) ((void)0)\n')
        (base/'esp_http_server.h').write_text(HEADER);(base/'test.c').write_text(HARNESS)
        asm=''
        for name in ('index_html','portal_css','portal_js'):
            asm+=f'.global _binary_{name}_start\n_binary_{name}_start:\n.byte 79,75\n.global _binary_{name}_end\n_binary_{name}_end:\n'
        (base/'assets.c').write_text('__asm__('+repr(asm).replace("'",'"')+');\n')
        cls.binary=base/'test'
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I'+str(base),'-Icomponents/portal/include',
            str(base/'test.c'),str(base/'assets.c'),'components/portal/streamoo_portal.c','components/portal/streamoo_portal_status.c',
            '-o',str(cls.binary)],cwd=ROOT,check=True)
    def run_case(self,mode): subprocess.run([str(self.binary),str(mode)],check=True,timeout=5)
    def test_routes_status_headers_and_lifecycle(self):self.run_case(0)
    def test_unavailable_snapshot(self):self.run_case(1)
    def test_partial_start_cleanup(self):self.run_case(2)
    def test_stop_failure_preserves_live_context(self):self.run_case(3)
