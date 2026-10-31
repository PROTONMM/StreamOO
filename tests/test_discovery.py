"""Real discovery owner with synthetic mDNS; no multicast or hardware claims."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class DiscoveryTest(unittest.TestCase):
    def test_lifecycle_failures_and_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = Path(tmp)
            (base/'mdns.h').write_text('''
#include <stddef.h>
#include <stdint.h>
typedef int esp_err_t;
#define ESP_OK 0
const char *esp_err_to_name(int e);
int mdns_init(void);
void mdns_free(void);
int mdns_register_hostname_changed_callback(void (*cb)(const char*,void*),void*);
int mdns_hostname_set(const char*);
int mdns_instance_name_set(const char*);
int mdns_service_add(const char*,const char*,const char*,uint16_t,void*,size_t);
''')
            (base/'esp_log.h').write_text('#include <stdio.h>\n#define ESP_LOGI(tag,...) ((void)printf(__VA_ARGS__))\n#define ESP_LOGW(tag,...) ((void)printf(__VA_ARGS__))\n')
            (base/'test.c').write_text(r'''
#include "mdns.h"
#include "streamoo_discovery.h"
#include <assert.h>
#include <string.h>
static int calls, frees, fail;
static int step(void){++calls;return calls==fail ? 1:0;}
const char *esp_err_to_name(int e){(void)e;return "fake";}
int mdns_init(void){return step();}
void mdns_free(void){++frees;}
int mdns_register_hostname_changed_callback(void (*cb)(const char*,void*),void *ctx){assert(cb);cb("streamoo-a1b2c3-2",ctx);return step();}
int mdns_hostname_set(const char *s){assert(!strcmp(s,"streamoo-a1b2c3"));return step();}
int mdns_instance_name_set(const char *s){assert(!strcmp(s,"StreamOO"));return step();}
int mdns_service_add(const char *i,const char *s,const char *p,uint16_t port,void *txt,size_t n){assert(!i&&!txt&&!n&&port==80&&!strcmp(s,"_http")&&!strcmp(p,"_tcp"));return step();}
int main(void){
 const char *name="StreamOO-A1B2C3";
 streamoo_discovery_update(false,name,0);
 streamoo_discovery_update(true,NULL,0);
 streamoo_discovery_update(true,"StreamOO-zzzzzz",0);
 streamoo_discovery_update(true,"StreamOO-123",0);
 assert(!calls&&!frees);
 for(int f=1;f<=5;++f){
  calls=0;fail=f;int before=frees;
  streamoo_discovery_update(true,name,0);assert(calls==f);
  assert(frees==before+(f>1));
  streamoo_discovery_update(true,name,29999999);assert(calls==f);
  streamoo_discovery_update(false,name,30000000);
 }
 calls=0;fail=0;
 streamoo_discovery_update(true,name,0);assert(calls==5);
 streamoo_discovery_update(true,name,60000000);assert(calls==5);
 int before=frees;
 streamoo_discovery_update(false,name,60000001);assert(frees==before+1);
 streamoo_discovery_update(false,name,60000002);assert(frees==before+1);
 calls=0;streamoo_discovery_update(true,name,60000003);assert(calls==5);
 streamoo_discovery_update(false,name,60000004);
 return 0;
}
''')
            binary = base/'probe'
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                            *os.environ.get('STREAMOO_DISCOVERY_SANITIZE','').split(),
                            '-I'+str(base),'-Icomponents/network',
                            'components/network/streamoo_discovery.c',str(base/'test.c'),
                            '-o',str(binary)],cwd=ROOT,check=True)
            subprocess.run([str(binary)],check=True)
