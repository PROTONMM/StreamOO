"""Run the actual player actor with a deterministic RTOS/HTTP/I2S schedule."""
from pathlib import Path
import os,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
HEADERS={
 'streamoo_settings.h':'#pragma once\n#include <stdbool.h>\n#include <stdint.h>\ntypedef struct{uint32_t list_timeout_s;bool list_autoselect,autoplay;char hostname[64];} streamoo_settings_t;\nstatic inline streamoo_settings_t streamoo_settings_get(void){return (streamoo_settings_t){.list_timeout_s=5,.list_autoselect=true};}\nstatic inline uint64_t streamoo_settings_last_station(void){return 0;}\nstatic inline void streamoo_settings_station_changed(uint64_t id){(void)id;}\n',
 'esp_err.h':'#pragma once\ntypedef int esp_err_t;\n#define ESP_OK 0\n#define ESP_FAIL -1\n#define ESP_ERR_INVALID_STATE 1\n#define ESP_ERR_INVALID_ARG 2\n#define ESP_ERR_NO_MEM 3\n#define ESP_ERR_NOT_SUPPORTED 5\nstatic inline const char *esp_err_to_name(int e){(void)e;return "error";}\n',
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
#include "streamoo_player_buffer.h"
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
static char failure_log[1024],diagnostic_log[1024],perf_log[1024],reader_log[2048],station_log[1024];
static unsigned station_changes;static bool transport_eof_test,reader_done;
static bool profile_test;static streamoo_stream_decode_fn measured_decode;static void *measured_context;static bool measured_once;
static void (*actor)(void *);static jmp_buf completed;
static unsigned ring_epoch,decoder_epoch;
static unsigned tick,opens,closes,head,tail,count,allocations;static size_t command_bytes;
static uint8_t messages[16][128];static int64_t now;
static bool flac_case;static size_t flac_queued=8192;static unsigned flac_reads,reads_before;
static int boot;
uint64_t fixture_last;bool fixture_autoplay;
static bool online,ap=true,output_live,force_fail,disable_meter,wrong_meter_generation,web_commands,web_retry;static uint64_t stop_gen,play_gen;static int manual;
static streamoo_audio_fill_fn output_fill;static void *output_context;
struct streamoo_http_stream{unsigned stopped;bool closing;};
struct streamoo_decoder{int unused;};
struct streamoo_catalog_snapshot{unsigned refs;streamoo_catalog_view_t view;};
static const streamoo_station_t entries[]={{.stable_id=11,.name=(const uint8_t *)"Zażółć A",.name_bytes=sizeof "Zażółć A"-1,.url=(const uint8_t *)"http://a.invalid"},{.stable_id=22,.name=(const uint8_t *)"Test B\x1b[31m",.name_bytes=sizeof "Test B\x1b[31m"-1,.url=(const uint8_t *)"http://b.invalid"}};
static const streamoo_station_t reversed[]={{.stable_id=22,.name=(const uint8_t *)"Test B\x1b[31m",.name_bytes=sizeof "Test B\x1b[31m"-1,.url=(const uint8_t *)"http://b.invalid"},{.stable_id=11,.name=(const uint8_t *)"Zażółć A",.name_bytes=sizeof "Zażółć A"-1,.url=(const uint8_t *)"http://a.invalid"}};
static streamoo_catalog_snapshot_t first={.view={entries,2,1}},second={.view={NULL,0,3}},reordered={.view={reversed,2,2}},*catalog=&first;
void fake_log(const char *tag,const char *fmt,...){(void)tag;va_list args;va_start(args,fmt);
 if(strstr(fmt,"Station changed")){++station_changes;vsnprintf(station_log,sizeof station_log,fmt,args);}
 else if(strstr(fmt,"Reader gen="))vsnprintf(reader_log,sizeof reader_log,fmt,args);
 else if(strstr(fmt,"Stream failed"))vsnprintf(failure_log,sizeof failure_log,fmt,args);
 else if(strstr(fmt,"Decode perf:"))vsnprintf(perf_log,sizeof perf_log,fmt,args);
 else if(strstr(fmt,"Decode diag"))vsnprintf(diagnostic_log,sizeof diagnostic_log,fmt,args);
 va_end(args);}
const char *streamoo_live_failure_name(streamoo_live_failure_t f){return f==STREAMOO_LIVE_SOURCE_EOF?"source_eof":f==STREAMOO_LIVE_DECODER_NO_PROGRESS?"decoder_no_progress":"none";}
int64_t esp_timer_get_time(void){return now;}
void *heap_caps_calloc(size_t n,size_t size,unsigned caps){(void)caps;void *p=calloc(n,size);if(p){++allocations;if(n==32)++ring_epoch;}return p;}
void heap_caps_free(void *p){if(p){assert(allocations);--allocations;free(p);}}
void *xQueueCreate(unsigned n,size_t size){assert(n==16&&size<=128);command_bytes=size;return (void *)1;}
void vQueueDelete(void *q){(void)q;}
int xQueueSend(void *q,const void *data,unsigned wait){(void)q;(void)wait;if(count==16)return 0;memcpy(messages[tail++%16],data,command_bytes);++count;return 1;}
int xQueueReceive(void *q,void *out,unsigned wait){(void)q;(void)wait;if(!count)return 0;memcpy(out,messages[head++%16],command_bytes);--count;return 1;}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *ctx,unsigned priority,void *handle){(void)name;(void)ctx;(void)handle;assert(priority==4&&stack>=24576);actor=fn;return 1;}
unsigned uxTaskGetStackHighWaterMark(void *task){(void)task;return 8000;}
streamoo_catalog_snapshot_t *streamoo_config_catalog_acquire(void){++catalog->refs;return catalog;}
streamoo_catalog_view_t streamoo_config_catalog_view(const streamoo_catalog_snapshot_t *s){return s?s->view:(streamoo_catalog_view_t){0};}
void streamoo_config_catalog_release(streamoo_catalog_snapshot_t *s){if(s){assert(s->refs);--s->refs;}}
bool streamoo_network_state(streamoo_network_state_t *s){*s=(streamoo_network_state_t){.sta_connected=online,.ap_ready=ap};return true;}
esp_err_t streamoo_http_stream_open(const char *url,streamoo_http_stream_t **s){assert(!strcmp(url,(manual==1&&opens==2)?"http://b.invalid":"http://a.invalid"));assert(online&&!ap);++opens;force_fail=false;reader_done=false;*s=heap_caps_calloc(1,sizeof **s,0);return *s?ESP_OK:ESP_ERR_NO_MEM;}
bool streamoo_http_stream_info(streamoo_http_stream_t *s,streamoo_http_stream_info_t *i){(void)s;*i=(streamoo_http_stream_info_t){.codec_known=true,.codec=flac_case?STREAMOO_CODEC_FLAC:STREAMOO_CODEC_MP3,.bitrate_kbps=flac_case?1500:192,.ended=transport_eof_test?reader_done:force_fail,.error=transport_eof_test?(reader_done?ESP_ERR_NOT_SUPPORTED:ESP_OK):force_fail?ESP_FAIL:ESP_OK};if(reader_done){strcpy(i->resource,"hls_reload");strcpy(i->stage,"parse");strcpy(i->reason,"master_after_media");i->hls=true;i->hls_segments=3;i->hls_reloads=1;}return true;}
streamoo_byte_source_t streamoo_http_stream_source(streamoo_http_stream_t *s){(void)s;return (streamoo_byte_source_t){0};}
size_t streamoo_http_stream_queued(streamoo_http_stream_t *s){(void)s;return flac_case?flac_queued:force_fail?0:8192;}
void streamoo_http_stream_stop(streamoo_http_stream_t *s){if(!s->closing){s->closing=true;s->stopped=tick;}}
bool streamoo_http_stream_dispose(streamoo_http_stream_t *s){assert(s->closing);if(tick-s->stopped<40)return false;assert(!output_live);heap_caps_free(s);return true;}
esp_err_t streamoo_decoder_open(streamoo_codec_t codec,streamoo_decoder_t **d){assert(codec==(flac_case?STREAMOO_CODEC_FLAC:STREAMOO_CODEC_MP3)&&ring_epoch>decoder_epoch);decoder_epoch=ring_epoch;*d=heap_caps_calloc(1,sizeof **d,0);return ESP_OK;}
void streamoo_decoder_close(streamoo_decoder_t *d){heap_caps_free(d);}
esp_err_t streamoo_decoder_process(streamoo_decoder_t *d,uint8_t *in,size_t n,bool eos,uint8_t *pcm,size_t cap,streamoo_decode_result_t *r){(void)d;(void)in;(void)n;(void)eos;(void)pcm;(void)cap;(void)r;if(profile_test){now+=125000;*r=(streamoo_decode_result_t){.consumed=n,.produced=16384};}return ESP_OK;}
bool streamoo_live_decode_init(streamoo_live_decode_t *p,streamoo_byte_source_t s,void *d,streamoo_stream_decode_fn f){(void)s;measured_once=false;measured_context=d;measured_decode=f;p->format=(streamoo_pcm_format_t){44100,2,16};return true;}
streamoo_stream_result_t streamoo_live_decode_read(streamoo_live_decode_t *p,int32_t *out){(void)p;if(profile_test&&!measured_once){uint8_t in=0,pcm=0;streamoo_decode_result_t result={0};assert(measured_decode(measured_context,&in,1,false,&pcm,1,&result)==ESP_OK);measured_once=true;}if(flac_case){++flac_reads;if(flac_queued==1)return STREAMOO_STREAM_WAIT;}if(force_fail){if(transport_eof_test)reader_done=true;p->failure=transport_eof_test?STREAMOO_LIVE_SOURCE_EOF:STREAMOO_LIVE_DECODER_NO_PROGRESS;p->last_input=19;return STREAMOO_STREAM_FAILED;}for(unsigned i=0;i<480;++i)out[i]=65536;return STREAMOO_STREAM_READY;}
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
 if(s.phase==STREAMOO_PLAYER_PLAYING)assert(!strcmp(s.codec,flac_case?"FLAC":"MP3")&&s.bitrate_kbps==(flac_case?1500u:192u));
 if(!s.running||s.phase==STREAMOO_PLAYER_WAIT_NETWORK||s.phase==STREAMOO_PLAYER_RETRY)assert(!s.codec[0]&&!s.bitrate_kbps);
 if(s.phase!=STREAMOO_PLAYER_PLAYING||disable_meter||wrong_meter_generation){assert(!s.metering.levels.valid);assert(!s.metering.spectrum_valid);}
 if(s.metering.spectrum_valid)assert(s.metering.generation==s.generation&&s.metering.bands[0]==12345);
 if(s.metering.levels.valid){assert(s.metering.generation==s.generation);assert(s.metering.levels.rms[0]==1234);}

 if(flac_case){
  if(tick==1){online=true;ap=false;assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,1}));}
  if(tick>=2&&tick<=5)assert(!output_live&&!decoder_epoch);
  if(tick==6)flac_queued=93750;
  if(tick==12){assert(output_live&&s.phase==STREAMOO_PLAYER_PLAYING);flac_queued=1;}
  if(tick==50){reads_before=flac_reads;flac_queued=1000;}
  if(tick==55){assert(flac_reads==reads_before&&opens==1);flac_queued=93750;}
  if(tick==63){assert(flac_reads>reads_before&&s.phase==STREAMOO_PLAYER_PLAYING&&opens==1);assert(streamoo_player_set_playing(false));}
  if(tick==130){assert(!output_live&&!allocations&&opens==1&&closes==1);longjmp(completed,1);}return;
 }
 if(boot){
  if(tick<=2){assert(!opens&&!output_live);assert(s.station_id==(boot==9?0:11));assert(s.running==(boot==8));}
  if(tick==3){online=true;ap=false;}
  if(boot==7&&tick==5){assert(!s.running&&!opens);assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_PLAY}));}
  if(tick==12){if(boot==9)assert(!s.running&&!s.station_id&&!opens);else assert(s.phase==STREAMOO_PLAYER_PLAYING&&opens==1);assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_STOP}));}
  if(tick==80){assert(!s.running&&!output_live&&!allocations);longjmp(completed,1);}return;
 }
 if(tick<=2)assert(!opens&&!output_live);
 if(tick==3){streamoo_player_spectrum_enabled(true);online=true;ap=false;assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,1}));}
 if(tick==9){assert(s.phase==STREAMOO_PLAYER_PLAYING&&s.station_id==11&&s.running);assert(station_changes==1&&strstr(station_log,"name=\"Zażółć A\"")&&!strstr(station_log,"http"));}
 if(tick==10){if(web_commands)assert(streamoo_player_set_playing(false));else assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_TOGGLE}));}
 if(web_commands&&tick==11){stop_gen=s.generation;assert(streamoo_player_set_playing(false));}
 if(web_commands&&tick==12)assert(s.generation==stop_gen);
 if(tick==12)assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){22,1}));
 if(tick==13)assert(station_changes==2&&strstr(station_log,"Test B?[31m")&&!strchr(station_log,27));
 if(tick==14)assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,1}));
 if(tick==49)assert(opens==1&&!output_live);
 if(tick==60){streamoo_player_spectrum_enabled(false);streamoo_player_state_t check;assert(streamoo_player_state(&check)&&!check.metering.spectrum_valid);}
 if(tick==65){assert(opens==2&&s.phase==STREAMOO_PLAYER_PLAYING);streamoo_player_spectrum_enabled(true);play_gen=s.generation;assert(streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,1}));}
 if(tick==66)assert(s.generation==play_gen&&opens==2&&s.phase==STREAMOO_PLAYER_PLAYING&&station_changes==3);
 if(web_commands&&tick==65){play_gen=s.generation;assert(streamoo_player_set_playing(true));assert(streamoo_player_set_playing(true));}
 if(web_commands&&tick==66){assert(s.generation==play_gen);assert(streamoo_player_set_volume(57));assert(streamoo_player_control(NULL,&(streamoo_control_command_t){.kind=STREAMOO_CONTROL_VOLUME_DELTA,.delta=3}));}
 if(web_commands&&tick==67){assert(s.volume==60&&s.volume_revision==2);assert(streamoo_player_set_volume(60));assert(!streamoo_player_set_volume(101));}
 if(web_commands&&tick==68){assert(s.volume_revision==2);assert(streamoo_player_set_volume(0));}
 if(web_commands&&tick==69){assert(s.volume==0);assert(streamoo_player_set_volume(100));}
 if(web_commands&&tick==70)assert(s.volume==100);
 if(tick==70)force_fail=true;
 if(tick==120){assert(strstr(failure_log,transport_eof_test?"stage=decode cause=source_eof":"stage=decode cause=decoder_no_progress"));assert(strstr(failure_log,"name=\"Zażółć A\"")&&!strstr(failure_log,"http://"));if(transport_eof_test)assert(strstr(reader_log,"error=error(5)")&&strstr(reader_log,"reason=master_after_media")&&strstr(reader_log,"segments=3 reloads=1"));assert(strstr(diagnostic_log,"in=19 used=0 out=0 needed=0"));online=false;ap=true;}
 if(tick==130){online=true;assert(!output_live);}
 if(tick==139)assert(s.phase==STREAMOO_PLAYER_WAIT_NETWORK);
 if(tick==140)ap=false;
 if(tick==150)assert(opens==2);
 if(web_retry&&tick==151)assert(streamoo_player_set_playing(true));
 if(web_retry&&tick==155)assert(opens==3&&s.phase==STREAMOO_PLAYER_PLAYING);
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
 if(tick==420){if(profile_test)assert(strstr(perf_log,"codec_max=125000us")&&strstr(perf_log,"block_max=125000us")&&strstr(perf_log,"pcm_at_max=16384"));assert(!s.running&&!s.station_id&&!output_live&&opens==3&&closes==3&&!allocations&&first.refs==0&&reordered.refs==0);longjmp(completed,1);}
}
int main(int argc,char **argv){assert(streamoo_player_reserve(false,1500)==8192);assert(streamoo_player_reserve(true,1500)==93750);assert(streamoo_player_reserve(true,0)==65536);assert(streamoo_player_reserve(true,1)==32768);assert(streamoo_player_reserve(true,UINT32_MAX)==196608);manual=argc>1?atoi(argv[1]):0;flac_case=manual==10;profile_test=manual==11;transport_eof_test=manual==12;disable_meter=manual==3;wrong_meter_generation=manual==4;web_commands=manual==5;web_retry=manual==6;if(manual>=7&&manual<=9){boot=manual;fixture_last=manual==9?99:11;fixture_autoplay=manual==8;}if(manual>=3)manual=0;assert(streamoo_player_start(50)==ESP_OK);assert(!streamoo_player_select(NULL,&(streamoo_ui_select_command_t){11,99}));assert(first.refs==0);if(!setjmp(completed))actor(NULL);return 0;}
'''
class NetworkPlayerTests(unittest.TestCase):
 def run_case(self,mode):
  with tempfile.TemporaryDirectory() as tmp:
   base=Path(tmp);(base/'freertos').mkdir()
   for name,body in HEADERS.items():
    if name=='streamoo_settings.h':
     body=body.replace('static inline streamoo_settings_t streamoo_settings_get(void){return (streamoo_settings_t){.list_timeout_s=5,.list_autoselect=true};}', 'extern uint64_t fixture_last;extern bool fixture_autoplay;static inline streamoo_settings_t streamoo_settings_get(void){return (streamoo_settings_t){.list_timeout_s=5,.list_autoselect=true,.autoplay=fixture_autoplay};}').replace('return 0;}\nstatic inline void streamoo_settings_station_changed', 'return fixture_last;}\nstatic inline void streamoo_settings_station_changed')
    (base/name).write_text(body)
   (base/'test.c').write_text(HARNESS)
   includes=['-I'+str(base)]+['-Icomponents/'+n+'/include' for n in ('metering','levels','player','configuration','stations','network','streaming','decoder','compressed_source','pcm_adapter','pcm','audio_gain','audio_output','ui','ui_widgets','input','graphics','text')]
   subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',*os.environ.get('STREAMOO_PLAYER_SANITIZE','').split(),*includes,str(base/'test.c'),'components/player/streamoo_player.c','components/pcm/streamoo_pcm.c','components/audio_gain/streamoo_audio_gain.c','-o',str(base/'player')],cwd=ROOT,check=True)
   subprocess.run([str(base/'player'),str(mode)],check=True,timeout=10)

 def test_actor_ap_gate_generation_switch_cancel_retry_and_removed_station(self):self.run_case(0)
 def test_manual_station_change_bypasses_previous_retry_delay(self):self.run_case(1)
 def test_manual_play_bypasses_previous_retry_delay(self):self.run_case(2)

 def test_meter_failure_does_not_stop_audio(self):self.run_case(3)
 def test_old_meter_generation_never_reaches_ui(self):self.run_case(4)

 def test_web_play_stop_idempotence_and_absolute_volume_with_encoder(self):self.run_case(5)
 def test_explicit_web_play_retries_without_toggling_stop(self):self.run_case(6)

 def test_restore_last_station_stop_autoplay_and_missing_id(self):
  for mode in (7,8,9):
   with self.subTest(mode=mode):self.run_case(mode)

 def test_flac_preroll_refill_keeps_same_session_and_reserves_pcm_first(self):self.run_case(10)

 def test_decode_wall_time_and_pcm_size_are_logged_from_actual_wrapper(self):self.run_case(11)

 def test_source_eof_log_refreshes_original_hls_error_and_station_name(self):self.run_case(12)
