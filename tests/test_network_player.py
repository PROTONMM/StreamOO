"""Run the actual player actor with a deterministic RTOS/HTTP/I2S schedule."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
HEADERS={
 'esp_err.h':'#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_STATE 1\n#define ESP_ERR_INVALID_ARG 2\n#define ESP_ERR_NO_MEM 3\nstatic inline const char *esp_err_to_name(int e){(void)e;return "error";}\n',
 'esp_attr.h':'#pragma once\n#define IRAM_ATTR\n#define DRAM_ATTR\n',
 'esp_log.h':'#pragma once\nvoid fake_log(const char *,const char *,...) __attribute__((format(printf,2,3)));\n#define ESP_LOGI fake_log\n#define ESP_LOGW fake_log\n',
 'esp_timer.h':'#pragma once\n#include <stdint.h>\nint64_t esp_timer_get_time(void);\n',
 'esp_heap_caps.h':'#pragma once\n#include <stddef.h>\n#define MALLOC_CAP_INTERNAL 1\n#define MALLOC_CAP_8BIT 2\n#define MALLOC_CAP_SPIRAM 4\nvoid *heap_caps_calloc(size_t,size_t,unsigned);\nvoid heap_caps_free(void *);\n',
 'sdkconfig.h':'#pragma once\n#define CONFIG_STREAMOO_I2S_DOUT 48\n#define CONFIG_STREAMOO_I2S_BCLK 47\n#define CONFIG_STREAMOO_I2S_LRCLK 21\n',
 'freertos/FreeRTOS.h':'#pragma once\ntypedef int portMUX_TYPE;\n#define portMUX_INITIALIZER_UNLOCKED 0\n#define portENTER_CRITICAL(p) ((void)(p))\n#define portEXIT_CRITICAL(p) ((void)(p))\n#define portENTER_CRITICAL_ISR(p) ((void)(p))\n#define portEXIT_CRITICAL_ISR(p) ((void)(p))\n#define pdTRUE 1\n#define pdPASS 1\n#define pdMS_TO_TICKS(x) (x)\n',
 'freertos/queue.h':'#pragma once\n#include <stddef.h>\ntypedef void *QueueHandle_t;\nvoid *xQueueCreate(unsigned,size_t);\nvoid vQueueDelete(void *);\nint xQueueSend(void *,const void *,unsigned);\nint xQueueReceive(void *,void *,unsigned);\n',
 'freertos/task.h':'#pragma once\nint xTaskCreate(void (*)(void *),const char *,unsigned,void *,unsigned,void *);\nvoid vTaskDelay(unsigned);\nunsigned uxTaskGetStackHighWaterMark(void *);\n'}
HARNESS=r'''
#include "streamoo_player.h"
#include "streamoo_config_store.h"
#include "streamoo_http_stream.h"
#include "streamoo_live_decode.h"
#include "streamoo_audio_output.h"
#include "streamoo_network.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
static char failure_log[1024],diagnostic_log[1024];
static void (*actor)(void *);static jmp_buf completed;
static unsigned tick,opens,closes,head,tail,count,allocations;static size_t command_bytes;
static uint8_t messages[16][128];static int64_t now;
static bool online,ap=true,output_live,force_fail,disable_meter,wrong_meter_generation;static int manual;
static streamoo_audio_fill_fn output_fill;static void *output_context;
struct streamoo_http_stream{unsigned stopped;bool closing;};
struct streamoo_decoder{int unused;};
struct streamoo_catalog_snapshot{unsigned refs;streamoo_catalog_view_t view;};
static const streamoo_station_t entries[]={{.stable_id=11,.url=(const uint8_t *)"http://a.invalid"},{.stable_id=22,.url=(const uint8_t *)"http://b.invalid"}};
static const streamoo_station_t reversed[]={{.stable_id=22,.url=(const uint8_t *)"http://b.invalid"},{.stable_id=11,.url=(const uint8_t *)"http://a.invalid"}};
static streamoo_catalog_snapshot_t first={.view={entries,2,1}},second={.view={NULL,0,3}},reordered={.view={reversed,2,2}},*catalog=&first;
void fake_log(const char *tag,const char *fmt,...){(void)tag;va_list args;va_start(args,fmt);
 if(strstr(fmt,"Stream failed"))vsnprintf(failure_log,sizeof failure_log,fmt,args);
 else if(strstr(fmt,"Decode diag"))vsnprintf(diagnostic_log,sizeof diagnostic_log,fmt,args);
 va_end(args);}
const char *streamoo_live_failure_name(streamoo_live_failure_t f){return f==STREAMOO_LIVE_DECODER_NO_PROGRESS?"decoder_no_progress":"none";}
int64_t esp_timer_get_time(void){return now;}
void *heap_caps_calloc(size_t n,size_t size,unsigned caps){(void)caps;void *p=calloc(n,size);if(p)++allocations;return p;}
void heap_caps_free(void *p){if(p){assert(allocations);--allocations;free(p);}}
void *xQueueCreate(unsigned n,size_t size){assert(n==16&&size<=128);command_bytes=size;return (void *)1;}
void vQueueDelete(void *q){(void)q;}
int xQueueSend(void *q,const void *data,unsigned wait){(void)q;(void)wait;if(count==16)return 0;memcpy(messages[tail++%16],data,command_bytes);++count;return 1;}
int xQueueReceive(void *q,void *out,unsigned wait){(void)q;(void)wait;if(!count)return 0;memcpy(out,messages[head++%16],command_bytes);--count;return 1;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *ctx,unsigned priority,void *handle){(void)name;(void)ctx;(void)handle;assert(priority==4&&stack>=8192);actor=fn;return 1;}
unsigned uxTaskGetStackHighWaterMark(void *task){(void)task;return 8000;}
streamoo_catalog_snapshot_t *streamoo_config_catalog_acquire(void){++catalog->refs;return catalog;}
streamoo_catalog_view_t streamoo_config_catalog_view(const streamoo_catalog_snapshot_t *s){return s?s->view:(streamoo_catalog_view_t){0};}
void streamoo_config_catalog_release(streamoo_catalog_snapshot_t *s){if(s){assert(s->refs);--s->refs;}}
bool streamoo_network_state(streamoo_network_state_t *s){*s=(streamoo_network_state_t){.sta_connected=online,.ap_ready=ap};return true;}
esp_err_t streamoo_http_stream_open(const char *url,streamoo_http_stream_t **s){assert(!strcmp(url,(manual==1&&opens==2)?"http://b.invalid":"http://a.invalid"));assert(online&&!ap);++opens;force_fail=false;*s=heap_caps_calloc(1,sizeof **s,0);return *s?ESP_OK:ESP_ERR_NO_MEM;}
bool streamoo_http_stream_info(streamoo_http_stream_t *s,streamoo_http_stream_info_t *i){(void)s;*i=(streamoo_http_stream_info_t){.codec_known=true,.codec=STREAMOO_CODEC_MP3,.ended=force_fail,.error=force_fail?ESP_FAIL:ESP_OK};return true;}
streamoo_byte_source_t streamoo_http_stream_source(streamoo_http_stream_t *s){(void)s;return (streamoo_byte_source_t){0};}
size_t streamoo_http_stream_queued(streamoo_http_stream_t *s){(void)s;return force_fail?0:8192;}
void streamoo_http_stream_stop(streamoo_http_stream_t *s){if(!s->closing){s->closing=true;s->stopped=tick;}}
bool streamoo_http_stream_dispose(streamoo_http_stream_t *s){assert(s->closing);if(tick-s->stopped<40)return false;assert(!output_live);heap_caps_free(s);return true;}
esp_err_t streamoo_decoder_open(streamoo_codec_t codec,streamoo_decoder_t **d){assert(codec==STREAMOO_CODEC_MP3);*d=heap_caps_calloc(1,sizeof **d,0);return ESP_OK;}
void streamoo_decoder_close(streamoo_decoder_t *d){heap_caps_free(d);}
esp_err_t streamoo_decoder_process(streamoo_decoder_t *d,uint8_t *in,size_t n,bool eos,uint8_t *pcm,size_t cap,streamoo_decode_result_t *r){(void)d;(void)in;(void)n;(void)eos;(void)pcm;(void)cap;(void)r;return ESP_OK;}
bool streamoo_live_decode_init(streamoo_live_decode_t *p,streamoo_byte_source_t s,void *d,streamoo_stream_decode_fn f){(void)s;(void)d;(void)f;p->format=(streamoo_pcm_format_t){44100,2,16};return true;}
streamoo_stream_result_t streamoo_live_decode_read(streamoo_live_decode_t *p,int32_t *out){(void)p;if(force_fail){p->failure=STREAMOO_LIVE_DECODER_NO_PROGRESS;p->last_input=19;return STREAMOO_STREAM_FAILED;}for(unsigned i=0;i<480;++i)out[i]=65536;return STREAMOO_STREAM_READY;}
esp_err_t streamoo_audio_output_open(streamoo_audio_pins_t p,streamoo_audio_fill_fn fn,void *ctx,streamoo_audio_output_t **out){(void)p;assert(!output_live&&online&&!ap);output_live=true;output_fill=fn;output_context=ctx;*out=(void *)1;return ESP_OK;}
esp_err_t streamoo_audio_output_close(streamoo_audio_output_t *out){assert(out&&output_live);output_live=false;++closes;return ESP_OK;}
bool streamoo_audio_output_stats(streamoo_audio_output_t *s,streamoo_audio_stats_t *out){(void)s;*out=(streamoo_audio_stats_t){tick,10,0};return true;}

struct streamoo_metering {
 streamoo_pcm_ring_t tap; streamoo_pcm_block_t storage[4];
 uint64_t generation; unsigned stopping;bool spectrum_enabled;
};
static streamoo_metering_t *active_meter;
esp_err_t streamoo_metering_open(uint64_t gen,streamoo_metering_t **out){
 if(disable_meter){*out=NULL;return ESP_ERR_NO_MEM;}
 *out=heap_caps_calloc(1,sizeof **out,0);active_meter=*out;
 (*out)->generation=gen;assert(streamoo_pcm_ring_init(&(*out)->tap,(*out)->storage,4));return ESP_OK;
}
void streamoo_metering_set_spectrum(streamoo_metering_t *m,bool enabled){if(m)m->spectrum_enabled=enabled;}
streamoo_pcm_ring_t *streamoo_metering_tap(streamoo_metering_t *m){return m?&m->tap:NULL;}
bool streamoo_metering_state(streamoo_metering_t *m,streamoo_metering_state_t *out){
 assert(m);*out=(streamoo_metering_state_t){.generation=m->generation+(wrong_meter_generation?1:0),.completed_us=now,
 .spectrum_valid=m->spectrum_enabled,.spectrum_completed_us=now,.bands={12345},
 .levels={.valid=true,.rms={1234,2345},.peak={3000,4000}}};return true;
}
bool streamoo_metering_dispose(streamoo_metering_t *m){
 if(!m)return true;
 assert(!output_live);
 if(!m->stopping){m->stopping=tick;return false;}
 if(tick-m->stopping<3)return false;
 active_meter=NULL;heap_caps_free(m);return true;
}
void vTaskDelay(unsigned ms){now+=(int64_t)ms*1000;++tick;
 if(output_live){int32_t samples[480];output_fill(output_context,samples,240);}
 if(active_meter){int32_t raw[480];while(streamoo_pcm_pop(&active_meter->tap,raw,NULL)){} }
 streamoo_player_state_t s;assert(streamoo_player_state(&s));
 if(s.phase!=STREAMOO_PLAYER_PLAYING||disable_meter||wrong_meter_generation){assert(!s.metering.levels.valid);assert(!s.metering.spectrum_valid);}
 if(s.metering.spectrum_valid)assert(s.metering.generation==s.generation&&s.metering.bands[0]==12345);
 if(s.metering.levels.valid){assert(s.metering.generation==s.generation);assert(s.metering.levels.rms[0]==1234);}

 if(tick<=2)assert(!opens&&!output_live);
 if(tick==3){streamoo_player_spectrum_enabled(true);online=true;ap=false;assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,1}));}
 if(tick==9)assert(s.phase==STREAMOO_PLAYER_PLAYING&&s.station_id==11&&s.running);
 if(tick==10)assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_TOGGLE}));
 if(tick==12)assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){22,1}));
 if(tick==14)assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,1}));
 if(tick==49)assert(opens==1&&!output_live);
 if(tick==60){streamoo_player_spectrum_enabled(false);streamoo_player_state_t check;assert(streamoo_player_state(&check)&&!check.metering.spectrum_valid);}
 if(tick==65){assert(opens==2&&s.phase==STREAMOO_PLAYER_PLAYING);streamoo_player_spectrum_enabled(true);}
 if(tick==70)force_fail=true;
 if(tick==120){assert(strstr(failure_log,"stage=decode cause=decoder_no_progress"));assert(strstr(diagnostic_log,"in=19 used=0 out=0 needed=0"));online=false;ap=true;}
 if(tick==130){online=true;assert(!output_live);}
 if(tick==139)assert(s.phase==STREAMOO_PLAYER_WAIT_NETWORK);
 if(tick==140)ap=false;
 if(tick==150)assert(opens==2);
 if(manual){
  if(tick==150&&manual==2)assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_TOGGLE}));
  if(tick==151){if(manual==1)assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){22,1}));else assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_TOGGLE}));}
  if(tick==155)assert(opens==3&&s.phase==STREAMOO_PLAYER_PLAYING&&s.station_id==(manual==1?22:11));
  if(tick==170)catalog=&second;
  if(tick==230){assert(!s.running&&!s.station_id&&!output_live&&opens==3&&closes==3&&!allocations&&first.refs==0);longjmp(completed,1);}
 }
 if(tick==320)assert(opens==3&&s.station_id==11&&s.phase==STREAMOO_PLAYER_PLAYING);
 if(tick==328)catalog=&reordered;
 if(tick==335)assert(opens==3&&s.station_id==11&&s.phase==STREAMOO_PLAYER_PLAYING);
 if(tick==340)catalog=&second;
 if(tick==420){assert(!s.running&&!s.station_id&&!output_live&&opens==3&&closes==3&&!allocations&&first.refs==0&&reordered.refs==0);longjmp(completed,1);}
}
int main(int argc,char **argv){manual=argc>1?atoi(argv[1]):0;disable_meter=manual==3;wrong_meter_generation=manual==4;if(manual>=3)manual=0;assert(streamoo_player_start(50)==ESP_OK);assert(!streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,99}));assert(first.refs==0);if(!setjmp(completed))actor(NULL);return 0;}
'''
class NetworkPlayerTests(unittest.TestCase):
 def run_case(self,mode):
  with tempfile.TemporaryDirectory() as tmp:
   base=Path(tmp);(base/'freertos').mkdir()
   for name,body in HEADERS.items():(base/name).write_text(body)
   (base/'test.c').write_text(HARNESS)
   includes=['-I'+str(base)]+['-Icomponents/'+n+'/include' for n in ('metering','levels','player','configuration','stations','network','streaming','decoder','compressed_source','pcm_adapter','pcm','audio_gain','audio_output','ui','ui_widgets','input','graphics','text')]
   subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_PLAYER_SANITIZE','').split(),*includes,str(base/'test.c'),'components/player/streamoo_player.c','components/pcm/streamoo_pcm.c','components/audio_gain/streamoo_audio_gain.c','-o',str(base/'player')],cwd=ROOT,check=True)
   subprocess.run([str(base/'player'),str(mode)],check=True,timeout=10)

 def test_actor_ap_gate_generation_switch_cancel_retry_and_removed_station(self):self.run_case(0)
 def test_manual_station_change_bypasses_previous_retry_delay(self):self.run_case(1)
 def test_manual_play_bypasses_previous_retry_delay(self):self.run_case(2)

 def test_meter_failure_does_not_stop_audio(self):self.run_case(3)
 def test_old_meter_generation_never_reaches_ui(self):self.run_case(4)
