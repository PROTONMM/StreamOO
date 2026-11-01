"""Actual network owner, deterministic ESP-IDF event/driver/NVS fakes."""
from pathlib import Path
import os,subprocess,tempfile,unittest
from test_network_player import HEADERS as COMMON
ROOT=Path(__file__).resolve().parents[1]
NET=r'''
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
typedef const char *esp_event_base_t;
typedef void *esp_event_handler_instance_t;
#define WIFI_EVENT ((esp_event_base_t)1)
#define IP_EVENT ((esp_event_base_t)2)
#define WIFI_EVENT_STA_DISCONNECTED 1
#define IP_EVENT_STA_GOT_IP 2
int esp_event_loop_create_default(void);
int esp_event_handler_instance_register(esp_event_base_t,int,void (*)(void *,esp_event_base_t,int32_t,void *),void *,void **);
int esp_event_handler_instance_unregister(esp_event_base_t,int,void *);
typedef struct {int index;} esp_netif_t;
typedef struct {uint32_t addr;} ip4_t;
typedef struct {ip4_t ip,gw,netmask;} esp_netif_ip_info_t;
#define ESP_IP4TOADDR(a,b,c,d) (((uint32_t)(a)<<24)|((uint32_t)(b)<<16)|((uint32_t)(c)<<8)|(d))
#define IPSTR "%u.%u.%u.%u"
#define IP2STR(ip) 10u,0u,0u,42u
int esp_netif_init(void);
esp_netif_t *esp_netif_create_default_wifi_ap(void);
esp_netif_t *esp_netif_create_default_wifi_sta(void);
void esp_netif_destroy_default_wifi(esp_netif_t *);
int esp_netif_get_ip_info(esp_netif_t *,esp_netif_ip_info_t *);
int esp_netif_set_ip_info(esp_netif_t *,const esp_netif_ip_info_t *);
int esp_netif_dhcps_start(esp_netif_t *);
int esp_netif_dhcps_stop(esp_netif_t *);
typedef int wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() 0
#define WIFI_MODE_STA 1
#define WIFI_MODE_APSTA 2
#define WIFI_IF_AP 1
#define WIFI_IF_STA 2
#define WIFI_AUTH_WPA2_PSK 2
#define WIFI_AUTH_OPEN 0
#define WIFI_FAST_SCAN 0
#define WIFI_ALL_CHANNEL_SCAN 1
#define WIFI_STORAGE_RAM 0
typedef enum {WIFI_PS_NONE,WIFI_PS_MIN_MODEM,WIFI_PS_MAX_MODEM} wifi_ps_type_t;
int esp_wifi_set_ps(wifi_ps_type_t);
int esp_wifi_get_ps(wifi_ps_type_t *);
#define ESP_MAC_WIFI_SOFTAP 1
#define ESP_ERR_WIFI_NOT_CONNECT 6
#define ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED 7
typedef struct {bool required;} pmf_t;
typedef union {
 struct {uint8_t ssid[32],password[64];struct {int authmode;} threshold;int scan_method;pmf_t pmf_cfg;} sta;
 struct {uint8_t ssid[32],password[64];unsigned ssid_len,channel;int authmode,max_connection;pmf_t pmf_cfg;} ap;
} wifi_config_t;
typedef struct {uint8_t ssid[33];int8_t rssi;} wifi_ap_record_t;
typedef struct {int num;} wifi_sta_list_t;
int esp_wifi_init(const wifi_init_config_t *);
int esp_wifi_deinit(void);
int esp_wifi_set_storage(int);
int esp_wifi_set_mode(int);
int esp_wifi_set_config(int,const wifi_config_t *);
int esp_wifi_start(void);
int esp_wifi_stop(void);
int esp_wifi_disconnect(void);
int esp_wifi_connect(void);
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *);
int esp_wifi_ap_get_sta_list(wifi_sta_list_t *);
int esp_read_mac(uint8_t *,int);
#include <sys/time.h>
typedef struct {bool wait_for_sync;unsigned num_of_servers;const char *servers[3];void (*sync_cb)(struct timeval *);} esp_sntp_config_t;
#define ESP_SNTP_SERVER_LIST(...) { __VA_ARGS__ }
#define ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(n,list) {.wait_for_sync=true,.num_of_servers=(n),.servers=list}
#define ESP_NETIF_SNTP_DEFAULT_CONFIG(s) ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(1,{s})
int esp_netif_sntp_init(const esp_sntp_config_t *);
'''
HARNESS=r'''
#include "streamoo_network.h"
#include "streamoo_config_store.h"
#include "net_fake.h"
#include "freertos/event_groups.h"
#include <assert.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int scenario,mode,attempts,ap_starts,ready_calls,stop_calls,saves,polls,idle_disconnects;
static bool radio_started,ap_configured,connected;
static wifi_ps_type_t ps_mode=WIFI_PS_MIN_MODEM;
static unsigned ps_sets,ps_gets;
static bool ps_confirmed,ps_warning;
static uint32_t bits;static int64_t now;
static char target[33],order[64];static void (*worker)(void *);static jmp_buf completed;
static void (*wifi_event)(void *,esp_event_base_t,int32_t,void *),(*ip_event)(void *,esp_event_base_t,int32_t,void *);
static void (*time_callback)(struct timeval *);
static streamoo_wifi_profiles_t persisted,queued;static bool has_queued;
void fake_log(const char *tag,const char *fmt,...){(void)tag;
 if(strstr(fmt,"Wi-Fi power save=NONE;"))ps_confirmed=true;
 if(strstr(fmt,"Wi-Fi power save NONE unconfirmed:"))ps_warning=true;}
int64_t esp_timer_get_time(void){return now;}
unsigned uxTaskGetStackHighWaterMark(void *p){(void)p;return 6000;}
void *xQueueCreate(unsigned count,size_t bytes){assert(count==1&&bytes==sizeof queued);return (void *)1;}
void vQueueDelete(void *p){(void)p;}
int xQueueSend(void *q,const void *data,unsigned wait){(void)q;(void)wait;if(has_queued)return 0;queued=*(const streamoo_wifi_profiles_t *)data;has_queued=true;return 1;}
int xQueueReceive(void *q,void *out,unsigned wait){(void)q;now+=(int64_t)wait*1000;++polls;
 if(polls==1&&scenario>=5&&scenario<12&&scenario!=8){streamoo_wifi_profiles_t p={0};if(scenario!=9){p.count=1;strcpy(p.profiles[0].ssid,"C");strcpy(p.profiles[0].password,"test-pass");}assert(streamoo_network_submit_profiles(&p)==ESP_OK);}
 if(scenario==13){
  streamoo_network_state_t s;assert(streamoo_network_state(&s));
  if(polls==1)assert(!s.time_synced);
  if(polls==2){struct timeval tv={.tv_sec=1793448000};time_callback(&tv);}
  if(polls==3){assert(s.time_synced);longjmp(completed,1);}
 }
 if(scenario==12){
  streamoo_network_state_t s;assert(streamoo_network_state(&s)&&s.sta_connected);
  if(polls==1)assert(s.rssi_valid&&s.rssi_dbm==-65);
  if(polls==2)assert(s.rssi_valid&&s.rssi_dbm==-80);
  if(polls==12)assert(s.rssi_valid&&s.rssi_dbm==0);
  if(polls==23){assert(!s.rssi_valid);longjmp(completed,1);}
 }
 if(polls==1&&scenario==4){connected=false;wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);}
 if(polls==1&&scenario>=14)longjmp(completed,1);
 if((scenario<=3&&polls==1)||(scenario==4&&polls==14)||((scenario==5||scenario==6||scenario==7||scenario==9||scenario==11)&&polls==2)||(scenario==10&&polls==60))longjmp(completed,1);
 if(has_queued){*(streamoo_wifi_profiles_t *)out=queued;has_queued=false;return 1;}return 0;
}
void vTaskDelay(unsigned t){now+=(int64_t)t*1000;}
void vTaskDelete(void *p){(void)p;longjmp(completed,1);}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *handle){(void)name;(void)arg;(void)handle;assert(priority==3&&stack>=8192);worker=fn;return 1;}
void *xEventGroupCreate(void){return &bits;}
void vEventGroupDelete(void *p){(void)p;}
uint32_t xEventGroupClearBits(void *p,uint32_t mask){(void)p;uint32_t old=bits;bits&=~mask;return old;}
uint32_t xEventGroupSetBits(void *p,uint32_t mask){(void)p;return bits|=mask;}
uint32_t xEventGroupGetBits(void *p){(void)p;return bits;}
uint32_t xEventGroupWaitBits(void *p,uint32_t mask,int clear,int all,unsigned timeout){(void)p;(void)all;(void)timeout;uint32_t found=bits&mask;if(clear)bits&=~mask;return found;}
int streamoo_config_store_init(void){return ESP_OK;}
bool streamoo_config_wifi_load(streamoo_wifi_profiles_t *p){*p=persisted;return true;}
int streamoo_config_wifi_save(const streamoo_wifi_profiles_t *p){if(!p->count){streamoo_network_state_t s;assert(streamoo_network_state(&s)&&!s.sta_connected);}++saves;if(scenario==7||scenario==11)return ESP_FAIL;persisted=*p;return ESP_OK;}
bool streamoo_wifi_profiles_valid(const streamoo_wifi_profiles_t *p){return p&&p->count<=8;}
int esp_netif_init(void){return ESP_OK;}
int esp_event_loop_create_default(void){return ESP_OK;}
int esp_event_handler_instance_register(esp_event_base_t base,int id,void (*fn)(void *,esp_event_base_t,int32_t,void *),void *ctx,void **instance){(void)id;(void)ctx;if(base==WIFI_EVENT)wifi_event=fn;else ip_event=fn;*instance=(void *)1;return ESP_OK;}
int esp_event_handler_instance_unregister(esp_event_base_t base,int id,void *h){(void)base;(void)id;(void)h;return ESP_OK;}
esp_netif_t *esp_netif_create_default_wifi_ap(void){static esp_netif_t n={1};return &n;}
esp_netif_t *esp_netif_create_default_wifi_sta(void){static esp_netif_t n={2};return &n;}
void esp_netif_destroy_default_wifi(esp_netif_t *n){(void)n;}
int esp_netif_get_ip_info(esp_netif_t *n,esp_netif_ip_info_t *ip){assert(n->index==2);*ip=(esp_netif_ip_info_t){.ip.addr=connected?1:0};return ESP_OK;}
int esp_netif_set_ip_info(esp_netif_t *n,const esp_netif_ip_info_t *ip){assert(n->index==1&&ip->ip.addr==ESP_IP4TOADDR(192,168,4,1));return ESP_OK;}
int esp_netif_dhcps_start(esp_netif_t *n){assert(n->index==1);return ESP_OK;}
int esp_netif_dhcps_stop(esp_netif_t *n){assert(n->index==1);return ESP_OK;}
int esp_wifi_init(const wifi_init_config_t *c){(void)c;return ESP_OK;}
int esp_wifi_deinit(void){return ESP_OK;}
int esp_wifi_set_storage(int storage){assert(storage==WIFI_STORAGE_RAM);return ESP_OK;}
int esp_wifi_set_mode(int next){if(radio_started&&next==WIFI_MODE_APSTA&&mode!=next){
 assert(ap_configured);assert(scenario==2||scenario==3||scenario==4||scenario==8||scenario==9||scenario==10||scenario==11);
 if(scenario==8)return ESP_FAIL;++ap_starts;
 if(scenario==2||scenario==10)assert(attempts>=2);if(scenario==4)assert(attempts>=3);
 }mode=next;return ESP_OK;}
int esp_wifi_set_config(int iface,const wifi_config_t *c){
 if(iface==WIFI_IF_AP){assert(!radio_started&&c->ap.authmode==WIFI_AUTH_WPA2_PSK&&c->ap.ssid_len==15&&!strcmp((const char *)c->ap.password,"test-only-key"));ap_configured=true;}
 else {assert(c->sta.scan_method==WIFI_FAST_SCAN);memcpy(target,c->sta.ssid,32);target[32]=0;}return ESP_OK;}
int esp_wifi_start(void){assert(ap_configured&&mode==WIFI_MODE_STA);radio_started=true;return ESP_OK;}
int esp_wifi_set_ps(wifi_ps_type_t ps){assert(radio_started&&!attempts&&ps==WIFI_PS_NONE);++ps_sets;
 if(scenario==14)return ESP_FAIL;
 if(scenario!=16)ps_mode=ps;
 return ESP_OK;}
int esp_wifi_get_ps(wifi_ps_type_t *ps){assert(ps&&ps_sets==1);++ps_gets;
 if(scenario==15)return ESP_FAIL;
 *ps=ps_mode;return ESP_OK;}
int esp_wifi_stop(void){radio_started=false;return ESP_OK;}
int esp_wifi_disconnect(void){if(!connected){++idle_disconnects;return ESP_OK;}connected=false;wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);return ESP_OK;}
int esp_wifi_connect(void){assert(attempts<63&&ps_sets==1&&ps_gets==1);order[attempts++]=target[0];
 bool ok=scenario==0||scenario==12||scenario==13||scenario>=14||((scenario>=5&&scenario<=9)&&scenario!=8)||scenario==4&&attempts==1||scenario==1&&target[0]=='B'||scenario==10&&target[0]=='C'||scenario==11&&target[0]=='C';
 if(scenario==6&&target[0]=='C')ok=false;
 connected=ok;if(ok)ip_event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);else wifi_event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);return ESP_OK;}
int esp_wifi_sta_get_ap_info(wifi_ap_record_t *p){if(!connected||(scenario==12&&polls>=20))return ESP_FAIL;memcpy(p->ssid,target,33);p->rssi=scenario==12?(polls>=10?3:polls>=1?-80:-65):-65;return ESP_OK;}
int esp_wifi_ap_get_sta_list(wifi_sta_list_t *p){p->num=0;return ESP_OK;}
int esp_read_mac(uint8_t *mac,int type){assert(type==ESP_MAC_WIFI_SOFTAP);memset(mac,0,6);mac[3]=0x12;mac[4]=0x34;mac[5]=0x56;return ESP_OK;}
int esp_netif_sntp_init(const esp_sntp_config_t *c){assert(!c->wait_for_sync&&c->num_of_servers==3&&c->sync_cb);assert(!strcmp(c->servers[0],"0.pool.ntp.org")&&!strcmp(c->servers[1],"1.pool.ntp.org")&&!strcmp(c->servers[2],"2.pool.ntp.org"));time_callback=c->sync_cb;struct timeval tv={.tv_sec=scenario==13?100:1793448000};c->sync_cb(&tv);return ESP_OK;}
static void ready(void *ctx){(void)ctx;++ready_calls;streamoo_network_state_t s;assert(streamoo_network_state(&s));assert(s.sta_connected||s.ap_ready);}
static void stopped(void *ctx){(void)ctx;++stop_calls;}
int main(int argc,char **argv){assert(argc==2);scenario=atoi(argv[1]);
 if(scenario!=3&&scenario!=11){persisted.count=2;strcpy(persisted.profiles[0].ssid,"A");strcpy(persisted.profiles[1].ssid,"B");strcpy(persisted.profiles[0].password,"test-pass");strcpy(persisted.profiles[1].password,"test-pass");}
 const streamoo_network_hooks_t hooks={ready,stopped,NULL};assert(streamoo_network_start(&hooks)==ESP_OK);
 if(!setjmp(completed))worker(NULL);
 assert(ps_sets==1&&ps_gets==1);
 if(scenario>=14){assert(ps_warning&&!ps_confirmed);}
 else {assert(ps_mode==WIFI_PS_NONE&&ps_confirmed&&!ps_warning);}
 streamoo_network_state_t s;assert(streamoo_network_state(&s));streamoo_network_ap_display_t screen;
 assert(streamoo_network_ap_display(&screen)==s.ap_ready);
 if(scenario==8){assert(!s.ap_ready&&!radio_started&&stop_calls==1&&!ready_calls);return 0;}
 assert(ready_calls==1&&!stop_calls&&!idle_disconnects);
 if(scenario>=14)assert(s.sta_connected&&!s.ap_ready&&!strcmp(order,"A"));
 if(!s.sta_connected)assert(!s.rssi_valid);
 if(scenario==0)assert(s.time_synced);
 if(scenario==0)assert(!strcmp(order,"A")&&s.sta_connected&&!s.ap_ready&&!ap_starts);
 if(scenario==1)assert(!strcmp(order,"AB")&&s.sta_connected&&!s.ap_ready&&!ap_starts);
 if(scenario==2)assert(!strcmp(order,"AB")&&!s.sta_connected&&s.ap_ready&&ap_starts==1);
 if(scenario==3)assert(!attempts&&s.ap_ready&&ap_starts==1);
 if(scenario==4)assert(!strcmp(order,"AAB")&&!s.sta_connected&&s.ap_ready&&ap_starts==1);
 if(scenario==5)assert(!strcmp(order,"AC")&&s.sta_connected&&!s.ap_ready&&!ap_starts&&saves==1&&persisted.count==1&&!s.last_error);
 if(scenario==6||scenario==7)assert(!strcmp(order,"ACA")&&s.sta_connected&&!s.ap_ready&&!ap_starts&&persisted.count==2&&s.last_error==1);
 if(scenario==9)assert(!strcmp(order,"A")&&!s.sta_connected&&s.ap_ready&&ap_starts==1&&persisted.count==0);
 if(scenario==10)assert(!strcmp(order,"ABC")&&s.sta_connected&&!s.ap_ready&&ap_starts==1&&persisted.count==1&&mode==WIFI_MODE_STA);
 if(scenario==11)assert(!strcmp(order,"C")&&!s.sta_connected&&!connected&&s.ap_ready&&ap_starts==1&&saves==1&&persisted.count==0&&s.last_error==1);
 return 0;}
'''
class NetworkStartupTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.addClassCleanup(cls.tmp.cleanup);base=Path(cls.tmp.name);(base/'freertos').mkdir()
  files=dict(COMMON);files['freertos/FreeRTOS.h']+='#define pdFALSE 0\n'
  files['esp_err.h']+='\n#define ESP_ERR_TIMEOUT 4\n#define ESP_ERR_NOT_SUPPORTED 5\n'
  files['sdkconfig.h']+='#define CONFIG_STREAMOO_NETWORK_ENABLED 1\n#define CONFIG_LWIP_SNTP_MAX_SERVERS 3\n'
  files['network_secret.h']='#pragma once\n#define STREAMOO_NETWORK_SECRET_AVAILABLE 1\nstatic const unsigned char streamoo_ap_password[]="test-only-key";\n'
  files['freertos/event_groups.h']='#pragma once\n#include <stdint.h>\ntypedef void *EventGroupHandle_t;\ntypedef uint32_t EventBits_t;\nvoid *xEventGroupCreate(void);\nvoid vEventGroupDelete(void *);\nuint32_t xEventGroupClearBits(void *,uint32_t);\nuint32_t xEventGroupSetBits(void *,uint32_t);\nuint32_t xEventGroupGetBits(void *);\nuint32_t xEventGroupWaitBits(void *,uint32_t,int,int,unsigned);\n'
  files['freertos/task.h']+='void vTaskDelete(void *);\n'
  files['net_fake.h']=NET
  for name in ('esp_event.h','esp_mac.h','esp_netif.h','esp_netif_sntp.h','esp_wifi.h','esp_wifi_default.h','nvs_flash.h'):files[name]='#include "net_fake.h"\n'
  files['esp_log.h']+='#define ESP_LOGE fake_log\n'
  for name,body in files.items():(base/name).write_text(body)
  (base/'test.c').write_text(HARNESS);cls.binary=base/'network'
  includes=['-I'+str(base)]+['-Icomponents/'+n+'/include' for n in ('network','configuration','stations')]
  (base/'discovery_stub.c').write_text('#include <stdbool.h>\n#include <stdint.h>\nvoid streamoo_discovery_update(bool connected,const char *name,int64_t now){(void)connected;(void)name;(void)now;}\n')
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-parentheses','-Wno-misleading-indentation',*os.environ.get('STREAMOO_NETWORK_SANITIZE','').split(),*includes,str(base/'test.c'),'components/network/streamoo_network.c','components/network/streamoo_network_policy.c',str(base/'discovery_stub.c'),'-o',str(cls.binary)],cwd=ROOT,check=True)
 def run_case(self,n):subprocess.run([str(self.binary),str(n)],check=True,timeout=10)
 def test_first_saved_network_without_ap(self):self.run_case(0)
 def test_second_saved_network_without_ap(self):self.run_case(1)
 def test_ap_only_after_all_profiles_fail(self):self.run_case(2)
 def test_no_profiles_immediate_secure_ap(self):self.run_case(3)
 def test_link_loss_exhausts_profiles_before_ap(self):self.run_case(4)
 def test_lan_edit_success_does_not_start_ap(self):self.run_case(5)
 def test_lan_edit_failure_restores_old_profile_before_ap(self):self.run_case(6)
 def test_commit_failure_restores_old_profile(self):self.run_case(7)
 def test_ap_driver_failure_cleanup(self):self.run_case(8)
 def test_empty_saved_list_starts_ap(self):self.run_case(9)
 def test_successful_ap_provisioning_disables_ap(self):self.run_case(10)

 def test_first_provisioning_commit_failure_disconnects_unsaved_station(self):self.run_case(11)

 def test_rssi_updates_positive_clamp_and_query_failure(self):self.run_case(12)

 def test_clock_valid_only_after_plausible_sntp_callback(self):self.run_case(13)

 def test_power_save_set_failure_keeps_network_available_and_warns(self):self.run_case(14)
 def test_power_save_query_failure_does_not_claim_success(self):self.run_case(15)
 def test_power_save_readback_mismatch_does_not_claim_success(self):self.run_case(16)
