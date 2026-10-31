#include "streamoo_player.h"
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
#include <inttypes.h>
#include <string.h>
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
static esp_err_t decode(void *d, uint8_t *in, size_t n, bool eos, uint8_t *pcm,
                        size_t cap, streamoo_decode_result_t *r) {
  return streamoo_decoder_process(d, in, n, eos, pcm, cap, r);
}
static void task(void *context) {
  (void)context;
  streamoo_catalog_snapshot_t *chosen = NULL;
  const streamoo_station_t *station = NULL;
  streamoo_http_stream_t *session = NULL;
  streamoo_decoder_t *decoder = NULL;
  streamoo_live_decode_t *pump = NULL;
  streamoo_audio_output_t *output = NULL;
  streamoo_pcm_block_t *storage = NULL, *block = NULL;
  streamoo_metering_t *meter = NULL;
  bool retiring = false;
  uint32_t sequence = 0, reader_hwm = 0;
  int64_t retry_at = 0, log_at = 0, fade_until = 0;
  unsigned backoff = 1;
  const char *failure_stage = "none";
  streamoo_player_state_t current;
  streamoo_player_state(&current);
  for (;;) {
    command_t cmd;
    bool restart = false;
    while (xQueueReceive(queue, &cmd, 0) == pdTRUE) {
      if (cmd.catalog) {
        streamoo_config_catalog_release(chosen);
        chosen = cmd.catalog;
        station = NULL;
        streamoo_catalog_view_t view = streamoo_config_catalog_view(chosen);
        for (uint32_t i = 0; i < view.count; ++i)
          if (view.stations[i].stable_id == cmd.id)
            station = &view.stations[i];
        current.station_id = station ? cmd.id : 0;
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
                 "Connect failed gen=%" PRIu64 " stage=http_open error=%s",
                 current.generation, esp_err_to_name(current.error));
        current.phase = STREAMOO_PLAYER_RETRY;
        retry_at = now + (int64_t)backoff * 1000000;
        backoff = backoff < 8 ? backoff * 2 : 15;
      } else
        ESP_LOGI("player", "Connect generation=%" PRIu64, current.generation);
    }
    if (session && !retiring && current.running && online) {
      streamoo_http_stream_info_t info = {0};
      streamoo_http_stream_info(session, &info);
      reader_hwm = info.reader_stack_hwm;
      size_t queued = streamoo_http_stream_queued(session);
      if (!decoder && info.codec_known &&
          (queued >= 8192 || (info.ended && queued))) {
        failure_stage = "decoder_open";
        current.error = streamoo_decoder_open(info.codec, &decoder);
        if (current.error == ESP_OK) {
          failure_stage = "memory";
          pump = heap_caps_calloc(1, sizeof *pump,
                                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
          storage = heap_caps_calloc(32, sizeof *storage,
                                     MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
          block = heap_caps_calloc(1, sizeof *block,
                                   MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
          if (!pump || !storage || !block)
            current.error = ESP_ERR_NO_MEM;
          else {
            esp_err_t meter_error = streamoo_metering_open(current.generation, &meter);
            if (meter_error != ESP_OK)
              ESP_LOGW("player", "Audio meters disabled: %s", esp_err_to_name(meter_error));
            failure_stage = "pipeline_init";
            if (!streamoo_live_decode_init(pump,
                                           streamoo_http_stream_source(session),
                                           decoder, decode) ||
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
        for (unsigned burst = 0; burst < 8 && streamoo_pcm_count(&ring) < 32;
             ++burst) {
          failure_stage = "decode";
          streamoo_stream_result_t r =
              streamoo_live_decode_read(pump, block->samples);
          if (r == STREAMOO_STREAM_WAIT)
            break;
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
          strcpy(current.codec,info.codec==STREAMOO_CODEC_MP3 ? "MP3" : "AAC");
          current.bitrate_kbps=info.bitrate_kbps;
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
        /* UART diagnostics must not delay the fade command. */
        fade_until = esp_timer_get_time() + 100000;
        ESP_LOGW("player",
                 "Stream failed gen=%" PRIu64 " stage=%s cause=%s"
                 " HTTP=%d error=%s decoder=%d vendor=%" PRId32
                 " previous=%" PRIu32 "/%u/%u format=%" PRIu32 "/%u/%u",
                 current.generation, failure_stage,
                 pump ? streamoo_live_failure_name(pump->failure) : "none",
                 info.http_status, esp_err_to_name(current.error),
                 pump ? pump->decoder_error : 0, pump ? pump->codec_status : 0,
                 pump ? pump->format.rate : 0,
                 pump ? (unsigned)pump->format.channels : 0,
                 pump ? (unsigned)pump->format.bits : 0,
                 pump ? pump->observed.rate : 0,
                 pump ? (unsigned)pump->observed.channels : 0,
                 pump ? (unsigned)pump->observed.bits : 0);
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
    if(retiring || !output || !online || !current.running){current.codec[0]=0;current.bitrate_kbps=0;}
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
    if (now - log_at >= 5000000) {
      streamoo_audio_stats_t audio = {0};
      if (output)
        streamoo_audio_output_stats(output, &audio);
      portENTER_CRITICAL(&lock);
      streamoo_pcm_stats_t pcm = stats;
      portEXIT_CRITICAL(&lock);
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
      if (meter) {
        streamoo_metering_state_t measurement = {0};
        streamoo_metering_state(meter, &measurement);
        ESP_LOGI("player", "meters valid=%u windows=%" PRIu64
                 " gaps=%" PRIu32 " drop=%" PRIu32 " max_work=%" PRIu32
                 "us stack_hwm=%" PRIu32 " rms=%u/%u peak=%u/%u",
                 (unsigned)measurement.levels.valid, measurement.levels.windows,
                 measurement.levels.discontinuities, pcm.tap_drops,
                 measurement.max_work_us, measurement.stack_hwm,
                 (unsigned)measurement.levels.rms[0], (unsigned)measurement.levels.rms[1],
                 (unsigned)measurement.levels.peak[0], (unsigned)measurement.levels.peak[1]);
      }
      if(meter) {
        streamoo_metering_state_t spectrum={0};streamoo_metering_state(meter,&spectrum);
        ESP_LOGI("player","FFT available=%u valid=%u frames=%" PRIu32
                 " skipped=%" PRIu32 " gaps=%" PRIu32 " max_fft=%" PRIu32 "us",
                 (unsigned)spectrum.spectrum_available,(unsigned)spectrum.spectrum_valid,
                 spectrum.fft_windows,spectrum.fft_skipped,spectrum.fft_gaps,spectrum.max_fft_us);
      }
      if (output && (audio.invalid_buffers || audio.max_fill_us >= 5000)) {
        failure_stage = "i2s_stats";
        current.error = ESP_FAIL;
      }
      log_at = now;
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
  if (xTaskCreate(task, "player", 12288, NULL, 4, NULL) != pdPASS) {
    vQueueDelete(queue);
    queue = NULL;
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}
