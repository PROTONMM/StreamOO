#include "streamoo_player.h"
#include "streamoo_player_buffer.h"
#include "streamoo_bitrate.h"
#include "streamoo_flac_backend.h"
#include "streamoo_settings.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "streamoo_audio_output.h"
#include "streamoo_config_store.h"
#include "streamoo_http_stream.h"
#include "streamoo_live_decode.h"
#include "streamoo_network.h"
#include "streamoo_pcm.h"
#include "streamoo_spectrum.h"
#include <inttypes.h>
#include <string.h>
#include <time.h>
typedef enum { COMMAND_ENCODER,COMMAND_PLAYING,COMMAND_VOLUME } command_kind_t;
typedef struct {
  command_kind_t kind;
  uint8_t value;
  streamoo_catalog_snapshot_t *catalog;
  uint64_t id;
  streamoo_control_command_t control;
} command_t;
static QueueHandle_t queue;
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
static streamoo_player_state_t state;
static DRAM_ATTR streamoo_pcm_ring_t ring;
static DRAM_ATTR streamoo_pcm_pipeline_t pipeline;
static uint32_t target_gain;
static bool spectrum_requested;
static streamoo_pcm_stats_t stats;
static void IRAM_ATTR fill(void *context, int32_t *lr, size_t frames) {
  portENTER_CRITICAL_ISR(&lock);
  uint32_t gain = target_gain;
  portEXIT_CRITICAL_ISR(&lock);
  streamoo_pcm_fill(context, lr, frames, gain);
  portENTER_CRITICAL_ISR(&lock);
  stats = ((streamoo_pcm_pipeline_t *)context)->stats;
  portEXIT_CRITICAL_ISR(&lock);
}
bool streamoo_player_state(streamoo_player_state_t *out) {
  if (!queue || !out)
    return false;
  portENTER_CRITICAL(&lock);
  *out = state;
  bool spectrum_on = spectrum_requested;
  portEXIT_CRITICAL(&lock);
  int64_t now = esp_timer_get_time();
  if (out->metering.levels.valid &&
      (now < out->metering.completed_us || now - out->metering.completed_us > 200000))
  {
    out->metering.levels.valid=false;
    memset(out->metering.levels.rms,0,sizeof out->metering.levels.rms);
    memset(out->metering.levels.peak,0,sizeof out->metering.levels.peak);
  }
  if(!spectrum_on || now<out->metering.spectrum_completed_us ||
     now-out->metering.spectrum_completed_us>200000) {
    out->metering.spectrum_valid=false;
    memset(out->metering.bands,0,sizeof out->metering.bands);
  }
  return true;
}
void streamoo_player_spectrum_enabled(bool enabled) {
  portENTER_CRITICAL(&lock);
  spectrum_requested=enabled;
  portEXIT_CRITICAL(&lock);
}
bool streamoo_player_select(void *context,
                            const streamoo_ui_select_command_t *selection) {
  (void)context;
  if (!queue || !selection)
    return false;
  streamoo_catalog_snapshot_t *snapshot = streamoo_config_catalog_acquire();
  streamoo_catalog_view_t view = streamoo_config_catalog_view(snapshot);
  bool found = false;
  if (view.generation == selection->catalog_generation)
    for (uint32_t i = 0; i < view.count; ++i)
      if (view.stations[i].stable_id == selection->station_id)
        found = true;
  command_t c = {.catalog = snapshot, .id = selection->station_id};
  if (found && xQueueSend(queue, &c, 0) == pdTRUE)
    return true;
  streamoo_config_catalog_release(snapshot);
  return false;
}
bool streamoo_player_control(void *context,
                             const streamoo_control_command_t *control) {
  (void)context;
  if(control && (control->kind==STREAMOO_CONTROL_PLAY||control->kind==STREAMOO_CONTROL_STOP))
    return streamoo_player_set_playing(control->kind==STREAMOO_CONTROL_PLAY);
  if (!queue || !control ||
      (control->kind != STREAMOO_CONTROL_TOGGLE &&
       control->kind != STREAMOO_CONTROL_VOLUME_DELTA))
    return false;
  command_t c = {.control = *control};
  return xQueueSend(queue, &c, 0) == pdTRUE;
}
bool streamoo_player_set_playing(bool running) {
  if(!queue)return false;
  command_t c={.kind=COMMAND_PLAYING,.value=running};
  return xQueueSend(queue,&c,0)==pdTRUE;
}
bool streamoo_player_set_volume(uint8_t volume) {
  if(!queue||volume>100)return false;
  command_t c={.kind=COMMAND_VOLUME,.value=volume};
  return xQueueSend(queue,&c,0)==pdTRUE;
}
static void station_label(char out[129],const streamoo_station_t *station){
  if(!station||!station->name||!station->name_bytes){strcpy(out,"<none>");return;}
  size_t n=station->name_bytes;if(n>128)n=128;
  if(n<station->name_bytes)while(n&&(station->name[n]&0xc0)==0x80)--n;
  for(size_t i=0;i<n;++i){uint8_t c=station->name[i];out[i]=c<32||c==127?'?':(char)c;}
  out[n]=0;
}
static void log_transport(const streamoo_http_stream_info_t *i,uint64_t generation){
  streamoo_network_state_t network={0};
  streamoo_network_state(&network);
  ESP_LOGW("player","TLS gen=%" PRIu64 " error=%s(%d) code=%d flags=0x%x errno=%d utc=%lld synced=%u",
    generation,esp_err_to_name(i->tls_error),i->tls_error,i->tls_code,(unsigned)i->tls_flags,
    i->socket_errno,(long long)time(NULL),(unsigned)network.time_synced);
  const char *resource=i->resource[0]?i->resource:"none",*stage=i->stage[0]?i->stage:"none",*reason=i->reason[0]?i->reason:"none";
  ESP_LOGW("player","Reader gen=%" PRIu64 " ended=%u error=%s(%d) resource=%s stage=%s reason=%s"
    " HTTP=%d mime=\"%s\" read=%d rx=%" PRIu64 "/%" PRIu64 " redirects=%" PRIu32
    " HLS=%u seq=%" PRIu64 " next=%" PRIu64 " media=%" PRIu64 "+%" PRIu32 " segments=%" PRIu32
    " reloads=%" PRIu32 " socket_errno=%d tls=%s(%d) tls_code=%d tls_flags=0x%x",
    generation,(unsigned)i->ended,esp_err_to_name(i->error),i->error,resource,stage,reason,
    i->http_status,i->content_type,i->last_read,i->response_bytes,i->received_bytes,i->redirects,
    (unsigned)i->hls,i->hls_sequence,i->hls_next,i->hls_media_sequence,i->hls_media_count,
    i->hls_segments,i->hls_reloads,i->socket_errno,esp_err_to_name(i->tls_error),i->tls_error,
    i->tls_code,(unsigned)i->tls_flags);
}
typedef struct {
  streamoo_decoder_t *decoder;
  streamoo_pcm_ring_t *ring;
  uint64_t codec_calls,codec_us;
  uint32_t codec_max_us,block_max_us,pcm_before_max,pcm_after_max;
  size_t pcm_max_bytes,pcm_at_max;
  size_t compressed_min,compressed_max;
  uint32_t pcm_min,pcm_max;
  uint32_t log_step_max_us;
  int64_t delivery_at;
  uint64_t delivery_bytes;
  streamoo_bitrate_t bitrate;
  bool levels_seen,flac;
} decode_profile_t;
static void log_delivery(decode_profile_t *p, const streamoo_http_stream_info_t *i) {
  int64_t now=esp_timer_get_time();
  uint64_t interval=now>p->delivery_at ? (uint64_t)(now-p->delivery_at) : 0;
  uint64_t bytes=i->received_bytes>=p->delivery_bytes ? i->received_bytes-p->delivery_bytes : 0;
  int64_t age=i->last_rx_us>0 && now>=i->last_rx_us ? (now-i->last_rx_us)/1000 : -1;
  /* HTTP body throughput, including ICY/manifest data; never codec bitrate.
   * Maxima are cumulative for this reader session. A full ring can itself
   * delay the next HTTP read, so interpret rx_age alongside push_wait_max. */
  ESP_LOGI("player","Delivery: interval_ms=%" PRIu64 " rx_delta=%" PRIu64
    " body_kbps=%" PRIu64 " rx_age_ms=%" PRId64 " read_max_us=%" PRIu64
    " push_wait_max_us=%" PRIu64 " log_step_max_us=%" PRIu32 " wait_empty=%" PRIu32 " wait_busy=%" PRIu32,
    interval/1000,bytes,interval?bytes*8000/interval:0,age,i->read_max_us,
    i->push_wait_max_us,p->log_step_max_us,i->source_empty_waits,i->source_busy_waits);
  p->delivery_at=now;p->delivery_bytes=i->received_bytes;
}
static uint32_t duration_us(int64_t start){
  int64_t elapsed=esp_timer_get_time()-start;
  return elapsed<=0?0:elapsed>UINT32_MAX?UINT32_MAX:(uint32_t)elapsed;
}
static esp_err_t decode(void *context,uint8_t *in,size_t n,bool eos,uint8_t *pcm,
                        size_t cap,streamoo_decode_result_t *r){
  decode_profile_t *p=context;
  uint32_t before=streamoo_pcm_count(p->ring);int64_t start=esp_timer_get_time();
  esp_err_t error=streamoo_decoder_process(p->decoder,in,n,eos,pcm,cap,r);
  uint32_t elapsed=duration_us(start);++p->codec_calls;p->codec_us+=elapsed;
  if(r->produced>p->pcm_max_bytes)p->pcm_max_bytes=r->produced;
  if(elapsed>p->codec_max_us){p->codec_max_us=elapsed;p->pcm_before_max=before;
    p->pcm_after_max=streamoo_pcm_count(p->ring);p->pcm_at_max=r->produced;}
  return error;
}
static void log_decode_perf(const decode_profile_t *p){
  ESP_LOGI("player","Decode perf: codec_max=%" PRIu32 "us block_max=%" PRIu32
    "us codec_total=%" PRIu64 "us calls=%" PRIu64 " backend=%s avg_call=%" PRIu64 "us",
    p->codec_max_us,p->block_max_us,p->codec_us,p->codec_calls,
    p->flac?streamoo_flac_backend_name():"espressif",p->codec_calls?p->codec_us/p->codec_calls:0);
}
static void log_decode_buffers(const decode_profile_t *p,const streamoo_live_decode_t *s){
  ESP_LOGI("player","Decode buffers: pcm_at_max=%zu pcm_max=%zu ring_at_max=%" PRIu32 "/%" PRIu32
    " src_wait=%" PRIu64 " budget_yield=%" PRIu64 " reads=%" PRIu64 " audio_kbps=%" PRIu32,
    p->pcm_at_max,p->pcm_max_bytes,
    p->pcm_before_max,p->pcm_after_max,s->source_waits,s->work_yields,s->source_calls,p->bitrate.kbps);
}
static void log_buffer_interval(decode_profile_t *p){
  if(p->levels_seen){
    ESP_LOGI("player","Buffer interval: compressed_min=%zu compressed_max=%zu PCM_min=%" PRIu32 " PCM_max=%" PRIu32,
      p->compressed_min,p->compressed_max,p->pcm_min,p->pcm_max);
    p->levels_seen=false;
  }
}
static void log_decode_profile(decode_profile_t *p,const streamoo_live_decode_t *s,bool refill){
  if(!s)return;
  log_decode_perf(p);
  log_decode_buffers(p,s);
  ESP_LOGI("player","Decode flow: fetched=%" PRIu64 " produced=%" PRIu64 " cache=%zu input=%zu refill=%u",
    s->fetched_total,s->produced_total,s->pcm_bytes-s->pcm_cursor,sizeof s->input,(unsigned)refill);
  log_buffer_interval(p);
}
static void task(void *context) {
  (void)context;
#ifdef ESP_PLATFORM
  ESP_LOGI("player", "Task placement: decoder/PCM core=%d (affinity=1)",
           xPortGetCoreID());
#endif
  streamoo_catalog_snapshot_t *chosen = NULL;
  const streamoo_station_t *station = NULL;
  streamoo_http_stream_t *session = NULL;
  streamoo_decoder_t *decoder = NULL;
  streamoo_live_decode_t *pump = NULL;
  decode_profile_t profile={0};
  streamoo_audio_output_t *output = NULL;
  streamoo_pcm_block_t *storage = NULL, *block = NULL;
  streamoo_metering_t *meter = NULL;
  bool retiring = false, flac_refill=false;
  uint32_t sequence = 0, reader_hwm = 0, logged_underflows = 0;
  int64_t retry_at = 0, log_at = 0, fade_until = 0;
  uint64_t log_generation=UINT64_MAX;
  unsigned log_step=0;
  unsigned backoff = 1;
  const char *failure_stage = "none";
  streamoo_player_state_t current;
  streamoo_player_state(&current);
  bool restored=false;
  uint64_t logged_station=0;char name[129]="<none>";
  for (;;) {
    if(!restored){
      streamoo_catalog_snapshot_t *initial=streamoo_config_catalog_acquire();
      if(initial){
        uint64_t id=streamoo_settings_last_station();streamoo_catalog_view_t view=streamoo_config_catalog_view(initial);
        for(uint32_t i=0;i<view.count;++i)if(view.stations[i].stable_id==id){
          chosen=initial;initial=NULL;station=&view.stations[i];current.station_id=id;
          current.running=streamoo_settings_get().autoplay;break;
        }
        streamoo_config_catalog_release(initial);restored=true;
      }
    }
    command_t cmd;
    bool restart = false;
    while (xQueueReceive(queue, &cmd, 0) == pdTRUE) {
      if (cmd.catalog) {
        if(cmd.id==current.station_id && current.running){
          streamoo_config_catalog_release(cmd.catalog);continue;
        }
        streamoo_config_catalog_release(chosen);
        chosen = cmd.catalog;
        station = NULL;
        streamoo_catalog_view_t view = streamoo_config_catalog_view(chosen);
        for (uint32_t i = 0; i < view.count; ++i)
          if (view.stations[i].stable_id == cmd.id)
            station = &view.stations[i];
        current.station_id = station ? cmd.id : 0;
        streamoo_settings_station_changed(current.station_id);
        current.running = station != NULL;
        ++current.generation;
        current.title[0] = 0;
        current.error = ESP_OK;
        backoff = 1;
        retry_at = 0;
        restart = true;
      } else if (cmd.kind==COMMAND_PLAYING ||
                 (cmd.kind==COMMAND_ENCODER && cmd.control.kind == STREAMOO_CONTROL_TOGGLE)) {
        bool desired=current.station_id && (cmd.kind==COMMAND_PLAYING ? cmd.value!=0:!current.running);
        if(desired==current.running){
          if(desired && cmd.kind==COMMAND_PLAYING && current.error!=ESP_OK &&
             (retiring || !session || current.phase==STREAMOO_PLAYER_RETRY)){
            current.error=ESP_OK;backoff=1;retry_at=0;
          }
          continue;
        }
        current.running = desired;
        ++current.generation;
        current.title[0] = 0;
        if (current.running) {
          current.error = ESP_OK;
          backoff = 1;
          retry_at = 0;
        }
        restart = true;
      } else {
        int64_t v = cmd.kind==COMMAND_VOLUME ? cmd.value:
                    (int64_t)current.volume + cmd.control.delta;
        uint8_t next = v < 0 ? 0 : v > 100 ? 100 : (uint8_t)v;
        if (next != current.volume) {
          current.volume = next;
          ++current.volume_revision;
        }
      }
    }
    /* Reconcile selection against the currently committed catalog. Never
     * replace a removed station with an unrelated one. */
    streamoo_catalog_snapshot_t *latest = streamoo_config_catalog_acquire();
    if (latest && latest != chosen && chosen) {
      streamoo_catalog_view_t view = streamoo_config_catalog_view(latest);
      const streamoo_station_t *replacement = NULL;
      for (uint32_t i = 0; i < view.count; ++i)
        if (view.stations[i].stable_id == current.station_id)
          replacement = &view.stations[i];
      streamoo_config_catalog_release(chosen);
      chosen = latest;
      latest = NULL;
      station = replacement;
      if (!station) {
        current.station_id = 0;
        streamoo_settings_station_changed(0);
        current.running = false;
        current.title[0] = 0;
        ++current.generation;
        restart = true;
      }
    }
    streamoo_config_catalog_release(latest);
    streamoo_network_state_t network = {0};
    streamoo_network_state(&network);
    bool online = network.sta_connected && !network.ap_ready;
    int64_t now = esp_timer_get_time();
    if (restart) {
      current.phase = current.running ? STREAMOO_PLAYER_CONNECTING
                                      : STREAMOO_PLAYER_STOPPED;
      current.rate = 0;
    }
    if ((restart || !online || !current.running) && session && !retiring) {
      retiring = true;
      fade_until = now + 100000;
      streamoo_http_stream_stop(session);
      portENTER_CRITICAL(&lock);
      target_gain = 0;
      portEXIT_CRITICAL(&lock);
    }
    station_label(name,station);
    if(logged_station!=current.station_id){
      ESP_LOGI("player","Station changed gen=%" PRIu64 " previous=%016" PRIx64 " id=%016" PRIx64 " name=\"%s\" running=%u",current.generation,logged_station,current.station_id,name,(unsigned)current.running);
      logged_station=current.station_id;
    }
    if (retiring) {
      if (now >= fade_until && output) {
        esp_err_t e = streamoo_audio_output_close(output);
        if (e != ESP_OK) {
          current.error = e;
          current.running = false;
          current.phase = STREAMOO_PLAYER_STOPPED;
          goto publish;
        }
        output = NULL;
      }
      bool meter_done = !output && streamoo_metering_dispose(meter);
      if (meter_done) meter = NULL;
      if (!output && meter_done && streamoo_http_stream_dispose(session)) {
        session = NULL;
        retiring = false;
        streamoo_decoder_close(decoder);
        decoder = NULL;
        heap_caps_free(pump);
        pump = NULL;
        profile=(decode_profile_t){0};
        heap_caps_free(storage);
        storage = NULL;
        heap_caps_free(block);
        block = NULL;
        retry_at =
            now + (current.error == ESP_OK ? 0 : (int64_t)backoff * 1000000);
        if (current.error != ESP_OK)
          backoff = backoff < 8 ? backoff * 2 : 15;
      }
    }
    if (!online) {
      current.phase = STREAMOO_PLAYER_WAIT_NETWORK;
      current.title[0] = 0;
    } else if (!current.running)
      current.phase = STREAMOO_PLAYER_STOPPED;
    else if (!session && now >= retry_at) {
      current.phase = STREAMOO_PLAYER_CONNECTING;
      current.rate = 0;
      current.title[0] = 0;
      ++current.generation;
      current.error =
          station
              ? streamoo_http_stream_open((const char *)station->url, &session)
              : ESP_ERR_INVALID_STATE;
      if (current.error != ESP_OK) {
        ESP_LOGW("player",
                 "Connect failed gen=%" PRIu64 " station=%016" PRIx64 " name=\"%s\" stage=http_open error=%s(%d)",
                 current.generation,current.station_id,name,esp_err_to_name(current.error),current.error);
        current.phase = STREAMOO_PLAYER_RETRY;
        retry_at = now + (int64_t)backoff * 1000000;
        backoff = backoff < 8 ? backoff * 2 : 15;
      } else
        ESP_LOGI("player", "Connect generation=%" PRIu64 " station=%016" PRIx64 " name=\"%s\"", current.generation,current.station_id,name);
    }
    if (session && !retiring && current.running && online) {
      streamoo_http_stream_info_t info = {0};
      streamoo_http_stream_info(session, &info);
      reader_hwm = info.reader_stack_hwm;
      size_t queued = streamoo_http_stream_queued(session);
      size_t reserve=streamoo_player_reserve(info.codec==STREAMOO_CODEC_FLAC,info.bitrate_kbps);
      if (!decoder && info.codec_known &&
          (queued >= reserve || (info.ended && queued))) {
        flac_refill=false;profile=(decode_profile_t){.flac=info.codec==STREAMOO_CODEC_FLAC};logged_underflows=0;
        profile.delivery_at=esp_timer_get_time();profile.delivery_bytes=info.received_bytes;
        /* Reserve the contiguous DMA/ISR ring before codec allocations can
         * fragment internal RAM. Large decode scratch belongs in PSRAM. */
        failure_stage = "pcm_memory";
        storage = heap_caps_calloc(32,sizeof *storage,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
        block = heap_caps_calloc(1,sizeof *block,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
        pump = heap_caps_calloc(1,sizeof *pump,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
        if(!pump||!storage||!block){
          current.error=ESP_ERR_NO_MEM;
          ESP_LOGW("player","Audio allocation failed: ring=%u block=%u scratch=%u",storage!=NULL,block!=NULL,pump!=NULL);
        }
        else {
          failure_stage="decoder_open";
          current.error=streamoo_decoder_open(info.codec,&decoder);
          if(current.error==ESP_OK) {
            esp_err_t meter_error = streamoo_metering_open(current.generation, &meter);
            if (meter_error != ESP_OK)
              ESP_LOGW("player", "Audio meters disabled: %s", esp_err_to_name(meter_error));
            failure_stage = "pipeline_init";
            profile.decoder=decoder;profile.ring=&ring;
            if (!streamoo_live_decode_init(pump,
                                           streamoo_http_stream_source(session),
                                           &profile, decode) ||
                !streamoo_pcm_ring_init(&ring, storage, 32) ||
                !streamoo_pcm_pipeline_init(&pipeline, &ring, streamoo_metering_tap(meter)))
              current.error = ESP_FAIL;
          }
        }
        if (current.error == ESP_OK) {
          failure_stage = "i2s_open";
          portENTER_CRITICAL(&lock);
          stats = (streamoo_pcm_stats_t){0};
          portEXIT_CRITICAL(&lock);
          const streamoo_audio_pins_t pins = {CONFIG_STREAMOO_I2S_DOUT,
                                              CONFIG_STREAMOO_I2S_BCLK,
                                              CONFIG_STREAMOO_I2S_LRCLK};
          current.error =
              streamoo_audio_output_open(pins, fill, &pipeline, &output);
          current.phase = STREAMOO_PLAYER_BUFFERING;
        }
      }
      if (current.error == ESP_OK && output) {
        uint32_t pcm_level=streamoo_pcm_count(&ring);
        if(!profile.levels_seen){profile.compressed_min=profile.compressed_max=queued;
          profile.pcm_min=profile.pcm_max=pcm_level;profile.levels_seen=true;}
        if(queued<profile.compressed_min)profile.compressed_min=queued;
        if(queued>profile.compressed_max)profile.compressed_max=queued;
        if(pcm_level<profile.pcm_min)profile.pcm_min=pcm_level;
        if(pcm_level>profile.pcm_max)profile.pcm_max=pcm_level;
        /* First owner observation of an ISR underflow, before replenishing
         * PCM. No logging or additional work is added to the ISR itself. */
        portENTER_CRITICAL(&lock);
        streamoo_pcm_stats_t before = stats;
        portEXIT_CRITICAL(&lock);
        if(before.underflows!=logged_underflows) {
          ESP_LOGW("player","Underrun gen=%" PRIu64 " low=%" PRIu32
                   " PCM=%" PRIu32 " compressed=%zu cache=%zu pending=%zu"
                   " wait_empty=%" PRIu32 " wait_busy=%" PRIu32 " refill=%u rx_age_ms=%" PRId64,
                   current.generation,before.underflows,streamoo_pcm_count(&ring),
                   queued,pump->pcm_bytes-pump->pcm_cursor,pump->pending,
                   info.source_empty_waits,info.source_busy_waits,(unsigned)flac_refill,
                   info.last_rx_us>0 && now>=info.last_rx_us ? (now-info.last_rx_us)/1000 : INT64_C(-1));
          logged_underflows=before.underflows;
        }
        if(flac_refill&&(queued>=reserve||info.ended))flac_refill=false;
        for (unsigned burst = 0; !flac_refill && burst < 8 && streamoo_pcm_count(&ring) < 32;
             ++burst) {
          failure_stage = "decode";
          int64_t block_at=esp_timer_get_time();
          streamoo_stream_result_t r = streamoo_live_decode_read(pump, block->samples);
          uint32_t block_us=duration_us(block_at);
          if(block_us>profile.block_max_us)profile.block_max_us=block_us;
          if (r == STREAMOO_STREAM_WAIT){
            if(info.codec==STREAMOO_CODEC_FLAC&&streamoo_pcm_count(&ring)<=STREAMOO_PCM_LOW_BLOCKS&&
               streamoo_http_stream_queued(session)<=1)flac_refill=true;
            break;
          }
          if (r != STREAMOO_STREAM_READY) {
            current.error = ESP_FAIL;
            break;
          }
          if (!streamoo_pcm_push(&ring, block->samples, sequence++)) {
            failure_stage = "pcm_queue";
            current.error = ESP_FAIL;
            break;
          }
        }
        current.rate = pump->format.rate;
        if(current.rate){
          const char *names[]={"MP3","AAC","FLAC","Vorbis","Opus","MPEG-TS"};
          strcpy(current.codec,info.codec<STREAMOO_CODEC_COUNT ? names[info.codec]:"?");
          uint32_t measured=streamoo_bitrate_update(&profile.bitrate,
              pump->consumed_total,pump->produced_total,pump->format.rate,
              pump->format.channels,pump->format.bits);
          current.bitrate_kbps=info.bitrate_kbps ? info.bitrate_kbps : measured;
          current.bitrate_measured=!info.bitrate_kbps && measured!=0;
        }
        portENTER_CRITICAL(&lock);
        streamoo_pcm_stats_t pcm = stats;
        portEXIT_CRITICAL(&lock);
        current.phase = pcm.mode == STREAMOO_PCM_ACTIVE
                            ? STREAMOO_PLAYER_PLAYING
                            : STREAMOO_PLAYER_BUFFERING;
        if (current.phase == STREAMOO_PLAYER_PLAYING)
          backoff = 1;
        memcpy(current.title, info.title, sizeof current.title);
      }
      if (info.ended && !queued && current.error == ESP_OK) {
        failure_stage = "http_end";
        current.error = info.error ? info.error : ESP_FAIL;
      }
      if (current.error != ESP_OK) {
        current.phase = STREAMOO_PLAYER_RETRY;
        current.title[0] = 0;
        retiring = true;
        streamoo_http_stream_stop(session);
        portENTER_CRITICAL(&lock);
        target_gain = 0;
        portEXIT_CRITICAL(&lock);
        /* UART diagnostics must not delay the fade command. Refresh after
         * source EOF so the reader's original terminal cause is not stale. */
        fade_until = esp_timer_get_time() + 100000;
        streamoo_http_stream_info(session,&info);
        ESP_LOGW("player",
                 "Stream failed gen=%" PRIu64 " stage=%s cause=%s"
                 " HTTP=%d codec=%u error=%s decoder=%s(%d) vendor=%" PRId32
                 " previous=%" PRIu32 "/%u/%u format=%" PRIu32 "/%u/%u station=%016" PRIx64 " name=\"%s\"",
                 current.generation, failure_stage,
                 pump ? streamoo_live_failure_name(pump->failure) : "none",
                 info.http_status, (unsigned)info.codec, esp_err_to_name(current.error),
                 esp_err_to_name(pump?pump->decoder_error:ESP_OK),
                 pump ? pump->decoder_error : 0, pump ? pump->codec_status : 0,
                 pump ? pump->format.rate : 0,
                 pump ? (unsigned)pump->format.channels : 0,
                 pump ? (unsigned)pump->format.bits : 0,
                 pump ? pump->observed.rate : 0,
                 pump ? (unsigned)pump->observed.channels : 0,
                 pump ? (unsigned)pump->observed.bits : 0,current.station_id,name);
        log_transport(&info,current.generation);
        log_decode_profile(&profile,pump,flac_refill);
        if (pump)
          ESP_LOGW("player",
                   "Decode diag gen=%" PRIu64
                   " in=%zu used=%zu out=%zu needed=%zu calls=%" PRIu64
                   " reads=%" PRIu64 " fetched=%" PRIu64 " consumed=%" PRIu64 " produced=%" PRIu64
                   " pending=%zu partial=%zu",
                   current.generation, pump->last_input, pump->last_consumed,
                   pump->last_produced, pump->last_needed, pump->decode_calls,
                   pump->source_calls, pump->fetched_total, pump->consumed_total,
                   pump->produced_total, pump->pending, pump->assembled);
      }
    }
    if (!session && current.running && online && now < retry_at)
      current.phase = STREAMOO_PLAYER_RETRY;
  publish: {
    if(retiring || !output || !online || !current.running){current.codec[0]=0;current.bitrate_kbps=0;current.bitrate_measured=false;}
    portENTER_CRITICAL(&lock);
    bool spectrum_on=spectrum_requested;
    portEXIT_CRITICAL(&lock);
    streamoo_metering_set_spectrum(meter,spectrum_on && !retiring &&
        online && current.running && current.phase==STREAMOO_PLAYER_PLAYING);
    current.metering = (streamoo_metering_state_t){0};
    if (meter && !retiring && current.phase == STREAMOO_PLAYER_PLAYING) {
      streamoo_metering_state(meter, &current.metering);
      if (current.metering.generation != current.generation)
        current.metering = (streamoo_metering_state_t){0};
    }
    uint32_t gain = 0;
    if (output && !retiring && current.running && online)
      streamoo_audio_gain_volume(current.volume, &gain);
    portENTER_CRITICAL(&lock);
    state = current;
    target_gain = gain;
    portEXIT_CRITICAL(&lock);
  }
    /* Periodic UART output is synchronous. Service PCM between individual
     * lines rather than blocking its producer for the whole report. Never
     * carry a partially printed report into another stream generation. */
    if(log_generation!=current.generation){log_generation=current.generation;log_step=0;}
    if (log_step || now - log_at >= 5000000) {
      int64_t log_started=esp_timer_get_time();
      portENTER_CRITICAL(&lock);
      streamoo_pcm_stats_t pcm = stats;
      portEXIT_CRITICAL(&lock);
      switch(log_step){
      case 0: {
        log_at=now;
        streamoo_audio_stats_t audio = {0};
        if (output)
          streamoo_audio_output_stats(output, &audio);
        ESP_LOGI("player",
                 "phase=%u gen=%" PRIu64 " rate=%" PRIu32
                 " compressed=%u PCM=%u max_fill=%" PRIu32 "us invalid=%" PRIu32
                 " low=%" PRIu32 " hard=%" PRIu32 " starts=%" PRIu32
                 " stack_hwm=%u reader_hwm=%u",
                 (unsigned)current.phase, current.generation, current.rate,
                 session ? (unsigned)streamoo_http_stream_queued(session) : 0,
                 output ? (unsigned)streamoo_pcm_count(&ring) : 0,
                 audio.max_fill_us, audio.invalid_buffers, pcm.underflows,
                 pcm.hard_underflows, pcm.restarts,
                 (unsigned)uxTaskGetStackHighWaterMark(NULL),
                 (unsigned)reader_hwm);
        if (output && (audio.invalid_buffers || audio.max_fill_us >= 5000)) {
          failure_stage = "i2s_stats";
          current.error = ESP_FAIL;
        }
        break;
      }
      case 1:
        if(pump)log_decode_perf(&profile);
        break;
      case 2:
        if(pump)log_decode_buffers(&profile,pump);
        break;
      case 3:
        log_buffer_interval(&profile);
        break;
      case 4:
        if(session && pump) {
          streamoo_http_stream_info_t delivery={0};
          if(streamoo_http_stream_info(session,&delivery))log_delivery(&profile,&delivery);
        }
        break;
      case 5:
        if (meter) {
          streamoo_metering_state_t measurement = {0};
          streamoo_metering_state(meter, &measurement);
          ESP_LOGI("player", "meters valid=%u windows=%" PRIu64
                   " gaps=%" PRIu32 " drop=%" PRIu32 " max_work=%" PRIu32
                   "us stack_hwm=%" PRIu32 " core_samples=%" PRIu32 "/%" PRIu32
                   " rms=%u/%u peak=%u/%u",
                   (unsigned)measurement.levels.valid, measurement.levels.windows,
                   measurement.levels.discontinuities, pcm.tap_drops,
                   measurement.max_work_us, measurement.stack_hwm,
                   measurement.core_samples[0], measurement.core_samples[1],
                   (unsigned)measurement.levels.rms[0], (unsigned)measurement.levels.rms[1],
                   (unsigned)measurement.levels.peak[0], (unsigned)measurement.levels.peak[1]);
        }
        break;
      case 6:
        if(meter) {
          streamoo_metering_state_t spectrum={0};streamoo_metering_state(meter,&spectrum);
          ESP_LOGI("player","FFT backend=%s available=%u valid=%u frames=%" PRIu32
                   " skipped=%" PRIu32 " gaps=%" PRIu32 " last_fft=%" PRIu32
                   "us avg_fft=%" PRIu64 "us max_fft=%" PRIu32 "us",
                   streamoo_spectrum_backend(),
                   (unsigned)spectrum.spectrum_available,(unsigned)spectrum.spectrum_valid,
                   spectrum.fft_windows,spectrum.fft_skipped,spectrum.fft_gaps,
                   spectrum.last_fft_us,
                   spectrum.fft_windows?spectrum.total_fft_us/spectrum.fft_windows:0,
                   spectrum.max_fft_us);
        }
        break;
      }
      log_step=(log_step+1)%7;
      uint32_t log_us=duration_us(log_started);
      if(log_us>profile.log_step_max_us)profile.log_step_max_us=log_us;
    }
    vTaskDelay(pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1);
  }
}
esp_err_t streamoo_player_start(uint8_t volume) {
  if (queue)
    return ESP_ERR_INVALID_STATE;
  if (volume > 100)
    return ESP_ERR_INVALID_ARG;
  queue = xQueueCreate(16, sizeof(command_t));
  if (!queue)
    return ESP_ERR_NO_MEM;
  state = (streamoo_player_state_t){.ready = true, .volume = volume};
#ifdef ESP_PLATFORM
  int created = xTaskCreatePinnedToCore(task, "player", 24576, NULL, 4,
                                         NULL, 1);
#else
  int created = xTaskCreate(task, "player", 24576, NULL, 4, NULL);
#endif
  if (created != pdPASS) {
    vQueueDelete(queue);
    queue = NULL;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}
