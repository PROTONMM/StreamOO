#include "streamoo_http_stream.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "streamoo_icy.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define CAPACITY (256u * 1024u)
static portMUX_TYPE info_lock = portMUX_INITIALIZER_UNLOCKED;
struct streamoo_http_stream {
  streamoo_buffered_source_t *buffer;
  uint8_t *storage;
  char url[2049], redirect_old[2049], redirect_next[2049];
  SemaphoreHandle_t done;
  atomic_uint stop;
  streamoo_http_stream_info_t info;
  streamoo_icy_t icy;
  uint32_t interval;
  bool bad_header, bad_location;
  uint8_t probe[3];
  size_t probe_bytes;
  struct {
    uint64_t offset;
    uint32_t revision;
    char title[257];
  } marks[64];
  unsigned mark_head, mark_count;
  uint32_t marked_revision;
  uint64_t consumed;
};
static bool url_allowed(const char *url) {
  if (strncmp(url, "http://", 7) && strncmp(url, "https://", 8))
    return false;
  size_t n = 0;
  for (; url[n] && n <= 2048; ++n)
    if ((unsigned char)url[n] <= 32 || (unsigned char)url[n] >= 127 ||
        url[n] == '@' || url[n] == '#')
      return false;
  return n > 7 && n <= 2048;
}
static bool mime(const char *value, const char *expected) {
  size_t n = strlen(expected);
  return !strncasecmp(value, expected, n) &&
         (!value[n] || value[n] == ';' || value[n] == ' ');
}
static esp_err_t header(esp_http_client_event_t *e) {
  streamoo_http_stream_t *s = e->user_data;
  if (e->event_id != HTTP_EVENT_ON_HEADER)
    return ESP_OK;
  if (!strcasecmp(e->header_key, "Location")) {
    if (strlen(e->header_value) > 2048 || strchr(e->header_value, '@'))
      s->bad_location = true;
  } else if (!strcasecmp(e->header_key, "icy-metaint")) {
    char *end = NULL;
    unsigned long n = strtoul(e->header_value, &end, 10);
    if (!*e->header_value || *end || !n || n > 16777216)
      s->bad_header = true;
    else
      s->interval = n;
  } else if (!strcasecmp(e->header_key, "Content-Type")) {
    streamoo_codec_t codec;
    bool known = true;
    if (mime(e->header_value, "audio/mpeg") ||
        mime(e->header_value, "audio/mp3") ||
        mime(e->header_value, "audio/x-mpeg") ||
        mime(e->header_value, "audio/x-mp3"))
      codec = STREAMOO_CODEC_MP3;
    else if (mime(e->header_value, "audio/aac") ||
             mime(e->header_value, "audio/aacp") ||
             mime(e->header_value, "audio/x-aac"))
      codec = STREAMOO_CODEC_AAC_LC;
    else {
      known = false;
      codec = STREAMOO_CODEC_MP3;
      if (!mime(e->header_value, "application/octet-stream"))
        s->bad_header = true;
    }
    portENTER_CRITICAL(&info_lock);
    s->info.codec_known = known;
    s->info.codec = codec;
    portEXIT_CRITICAL(&info_lock);
  }
  return ESP_OK;
}
static bool write_audio(streamoo_http_stream_t *s, const uint8_t *data,
                        size_t n) {
  size_t offset = 0;
  while (offset < n && !atomic_load(&s->stop)) {
    offset +=
        streamoo_buffered_source_write(s->buffer, 1, data + offset, n - offset);
    if (offset < n)
      vTaskDelay(pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1);
  }
  return offset == n;
}
static void reader(void *context) {
  streamoo_http_stream_t *s = context;
  esp_http_client_config_t config = {.url = s->url,
                                     .timeout_ms = 3000,
                                     .buffer_size = 2048,
                                     .buffer_size_tx = 1024,
                                     .disable_auto_redirect = true,
                                     .event_handler = header,
                                     .user_data = s,
                                     .crt_bundle_attach = esp_crt_bundle_attach,
                                     .user_agent = "StreamOO/1",
                                     .auth_type = HTTP_AUTH_TYPE_NONE};
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t error = client ? ESP_OK : ESP_ERR_NO_MEM;
  bool accepted = false;
  if (client) {
    esp_http_client_set_header(client, "Icy-MetaData", "1");
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
  }
  for (unsigned redirects = 0;
       error == ESP_OK && !atomic_load(&s->stop) && redirects <= 5;
       ++redirects) {
    s->interval = 0;
    s->bad_header = false;
    s->bad_location = false;
    portENTER_CRITICAL(&info_lock);
    s->info.codec_known = false;
    portEXIT_CRITICAL(&info_lock);
    error = esp_http_client_open(client, 0);
    if (error != ESP_OK)
      break;
    if (esp_http_client_fetch_headers(client) < 0) {
      error = ESP_FAIL;
      break;
    }
    int code = esp_http_client_get_status_code(client);
    portENTER_CRITICAL(&info_lock);
    s->info.http_status = code;
    portEXIT_CRITICAL(&info_lock);
    if (code == 200) {
      accepted = true;
      break;
    }
    if (!(code == 301 || code == 302 || code == 303 || code == 307 ||
          code == 308) ||
        redirects == 5) {
      error = ESP_ERR_INVALID_RESPONSE;
      break;
    }
    if (s->bad_location) {
      error = ESP_ERR_INVALID_RESPONSE;
      break;
    }
    char *old = s->redirect_old, *next = s->redirect_next;
    if (esp_http_client_get_url(client, old, sizeof s->redirect_old) !=
            ESP_OK ||
        esp_http_client_set_redirection(client) != ESP_OK ||
        esp_http_client_get_url(client, next, sizeof s->redirect_next) !=
            ESP_OK ||
        !url_allowed(next) ||
        (!strncmp(old, "https://", 8) && strncmp(next, "https://", 8))) {
      error = ESP_ERR_INVALID_RESPONSE;
      break;
    }
    esp_http_client_close(client);
  }
  if (!accepted || s->bad_header || !streamoo_icy_init(&s->icy, s->interval))
    error = ESP_ERR_INVALID_RESPONSE;
  uint8_t input[1024], audio[1024];
  size_t total = 0;
  unsigned timeouts = 0, reads = 0;
  while (error == ESP_OK && !atomic_load(&s->stop)) {
    int n = esp_http_client_read(client, (char *)input, sizeof input);
    if (n == -ESP_ERR_HTTP_EAGAIN) {
      if (++timeouts <= 4)
        continue;
      error = ESP_ERR_TIMEOUT;
      break;
    }
    timeouts = 0;
    if (n <= 0) {
      error = n < 0 ? ESP_FAIL : ESP_ERR_INVALID_RESPONSE;
      break;
    }
    if ((++reads & 63u) == 1) {
      uint32_t hwm = uxTaskGetStackHighWaterMark(NULL);
      portENTER_CRITICAL(&info_lock);
      s->info.reader_stack_hwm = hwm;
      portEXIT_CRITICAL(&info_lock);
    }
    size_t bytes = 0;
    if (!streamoo_icy_feed(&s->icy, input, n, audio, sizeof audio, &bytes)) {
      error = ESP_ERR_INVALID_RESPONSE;
      break;
    }
    if (bytes) {
      portENTER_CRITICAL(&info_lock);
      if (!s->info.codec_known) {
        size_t take = 3 - s->probe_bytes;
        if (take > bytes)
          take = bytes;
        memcpy(s->probe + s->probe_bytes, audio, take);
        s->probe_bytes += take;
      }
      if (!s->info.codec_known && s->probe_bytes == 3) {
        if (s->probe[0] == 0xff && (s->probe[1] & 0xf6) == 0xf0) {
          s->info.codec = STREAMOO_CODEC_AAC_LC;
          s->info.codec_known = true;
        } else if (!memcmp(s->probe, "ID3", 3) ||
                   (s->probe[0] == 0xff && (s->probe[1] & 0xe0) == 0xe0)) {
          s->info.codec = STREAMOO_CODEC_MP3;
          s->info.codec_known = true;
        }
      }
      bool known = s->info.codec_known;
      portEXIT_CRITICAL(&info_lock);
      total += bytes;
      if (!known && total > 32768) {
        error = ESP_ERR_NOT_SUPPORTED;
        break;
      }
      if (!write_audio(s, audio, bytes))
        break;
    }
    portENTER_CRITICAL(&info_lock);
    if (s->marked_revision != s->icy.revision && s->mark_count < 64) {
      unsigned at = (s->mark_head + s->mark_count) % 64;
      s->marks[at].offset = s->icy.title_audio_offset;
      s->marks[at].revision = s->icy.revision;
      memcpy(s->marks[at].title, s->icy.title, sizeof s->icy.title);
      ++s->mark_count;
      s->marked_revision = s->icy.revision;
    }
    portEXIT_CRITICAL(&info_lock);
    /* Also yield on metadata-heavy or very fast sources with no backpressure. */
    if ((reads & 15u) == 0)
      vTaskDelay(1);
  }
  if (client)
    esp_http_client_cleanup(client);
  while (!atomic_load(&s->stop) &&
         !streamoo_buffered_source_finish(s->buffer, 1, false))
    vTaskDelay(1);
  portENTER_CRITICAL(&info_lock);
  s->info.ended = true;
  s->info.error = error;
  portEXIT_CRITICAL(&info_lock);
  xSemaphoreGive(s->done);
  vTaskDelete(NULL);
}
esp_err_t streamoo_http_stream_open(const char *url,
                                    streamoo_http_stream_t **out) {
  if (!out || !url || !url_allowed(url))
    return ESP_ERR_INVALID_ARG;
  *out = NULL;
  streamoo_http_stream_t *s =
      heap_caps_calloc(1, sizeof *s, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!s)
    return ESP_ERR_NO_MEM;
  atomic_init(&s->stop, 0);
  strcpy(s->url, url);
  s->storage = heap_caps_malloc(CAPACITY, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  s->done = xSemaphoreCreateBinary();
  if (s->storage)
    s->buffer = streamoo_buffered_source_create(s->storage, CAPACITY);
  if (!s->done || !s->buffer ||
      xTaskCreate(reader, "http_reader", 10240, s, 3, NULL) != pdPASS) {
    if (s->done)
      vSemaphoreDelete(s->done);
    streamoo_buffered_source_destroy(s->buffer);
    heap_caps_free(s->storage);
    heap_caps_free(s);
    return ESP_ERR_NO_MEM;
  }
  *out = s;
  return ESP_OK;
}
bool streamoo_http_stream_info(streamoo_http_stream_t *s,
                               streamoo_http_stream_info_t *out) {
  if (!s || !out)
    return false;
  portENTER_CRITICAL(&info_lock);
  *out = s->info;
  portEXIT_CRITICAL(&info_lock);
  return true;
}
static streamoo_source_result_t source_read(void *context, uint8_t *data,
                                            size_t cap, size_t *bytes,
                                            bool *last) {
  streamoo_http_stream_t *s = context;
  streamoo_source_result_t result =
      streamoo_buffered_source_read(s->buffer, data, cap, bytes, last);
  if (result == STREAMOO_SOURCE_DATA) {
    portENTER_CRITICAL(&info_lock);
    s->consumed += *bytes;
    bool changed = false;
    unsigned latest = 0;
    while (s->mark_count && s->marks[s->mark_head].offset <= s->consumed) {
      latest = s->mark_head;
      changed = true;
      s->mark_head = (s->mark_head + 1) % 64;
      --s->mark_count;
    }
    if (changed) {
      s->info.metadata_revision = s->marks[latest].revision;
      memcpy(s->info.title, s->marks[latest].title, sizeof s->info.title);
    }
    portEXIT_CRITICAL(&info_lock);
  }
  return result;
}
streamoo_byte_source_t streamoo_http_stream_source(streamoo_http_stream_t *s) {
  return (streamoo_byte_source_t){s, source_read, NULL};
}
size_t streamoo_http_stream_queued(streamoo_http_stream_t *s) {
  streamoo_buffered_source_stats_t stats = {0};
  streamoo_buffered_source_stats(s->buffer, &stats);
  return stats.queued;
}
void streamoo_http_stream_stop(streamoo_http_stream_t *s) {
  if (s) {
    atomic_store(&s->stop, 1);
    streamoo_buffered_source_stop(s->buffer);
  }
}
bool streamoo_http_stream_dispose(streamoo_http_stream_t *s) {
  if (!s)
    return true;
  if (xSemaphoreTake(s->done, 0) != pdTRUE)
    return false;
  streamoo_buffered_source_destroy(s->buffer);
  vSemaphoreDelete(s->done);
  heap_caps_free(s->storage);
  heap_caps_free(s);
  return true;
}
