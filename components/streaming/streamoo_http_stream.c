#include "streamoo_http_stream.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "streamoo_hls.h"
#include "streamoo_icy.h"
#include "streamoo_playlist_resolver.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#define CAPACITY (256u * 1024u)
static portMUX_TYPE info_lock = portMUX_INITIALIZER_UNLOCKED;
struct streamoo_http_stream {
  streamoo_buffered_source_t *buffer;
  uint8_t *storage;
  char url[2049], request_url[2049], location[2049], redirect_old[2049],
      redirect_next[2049];
  SemaphoreHandle_t done;
  atomic_uint stop;
  streamoo_http_stream_info_t info;
  streamoo_icy_t icy;
  uint32_t interval;
  const char *header_reason;
  bool bad_header, bad_location, playlist_header, hls_active;
  bool response_known;
  streamoo_codec_t response_codec;
  uint8_t probe[512];
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
static void copy_tag(char *out, size_t capacity, const char *value) {
  size_t n = strlen(value);
  if (n >= capacity)
    n = capacity - 1;
  memcpy(out, value, n);
  out[n] = 0;
}
static void trace(streamoo_http_stream_t *s, const char *resource,
                  const char *stage) {
  portENTER_CRITICAL(&info_lock);
  if (resource)
    copy_tag(s->info.resource, sizeof s->info.resource, resource);
  if (stage)
    copy_tag(s->info.stage, sizeof s->info.stage, stage);
  portEXIT_CRITICAL(&info_lock);
}
static esp_err_t fail(streamoo_http_stream_t *s, const char *reason,
                      esp_err_t error) {
  portENTER_CRITICAL(&info_lock);
  if (!s->info.reason[0])
    copy_tag(s->info.reason, sizeof s->info.reason, reason);
  portEXIT_CRITICAL(&info_lock);
  return error;
}
static void response_begin(streamoo_http_stream_t *s) {
  s->interval = 0;
  s->bad_header = false;
  s->bad_location = false;
  s->location[0] = 0;
  s->playlist_header = false;
  s->response_known = false;
  s->header_reason = NULL;
  portENTER_CRITICAL(&info_lock);
  s->info.http_status = 0;
  s->info.response_bytes = 0;
  s->info.last_read = 0;
  s->info.content_type[0] = 0;
  portEXIT_CRITICAL(&info_lock);
  trace(s, NULL, "open");
}
static int read_http(streamoo_http_stream_t *s, esp_http_client_handle_t client,
                     char *data, int capacity) {
  int64_t started = esp_timer_get_time();
  int n = esp_http_client_read(client, data, capacity);
  int64_t completed = esp_timer_get_time();
  uint64_t elapsed = completed >= started ? (uint64_t)(completed - started) : 0;
  portENTER_CRITICAL(&info_lock);
  s->info.last_read = n;
  if (elapsed > s->info.read_max_us) s->info.read_max_us = elapsed;
  if (n > 0) {
    s->info.last_rx_us = completed;
    s->info.response_bytes += (unsigned)n;
    s->info.received_bytes += (unsigned)n;
  }
  bool measure = (++s->info.read_calls & 63u) == 1;
  portEXIT_CRITICAL(&info_lock);
  if (measure) {
    uint32_t hwm = uxTaskGetStackHighWaterMark(NULL);
    portENTER_CRITICAL(&info_lock);
    s->info.reader_stack_hwm = hwm;
    portEXIT_CRITICAL(&info_lock);
  }
  return n;
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
    if (!*e->header_value || strlen(e->header_value) > 2048 ||
        strchr(e->header_value, '@') || s->location[0])
      s->bad_location = true;
    else
      strcpy(s->location, e->header_value);
  } else if (!strcasecmp(e->header_key, "icy-metaint")) {
    char *end = NULL;
    unsigned long n = strtoul(e->header_value, &end, 10);
    if (!*e->header_value || *end || !n || n > 16777216) {
      s->bad_header = true;
      s->header_reason = "invalid_icy_interval";
    } else
      s->interval = n;
  } else if (!strcasecmp(e->header_key, "icy-br")) {
    uint32_t kbps = 0;
    bool valid = *e->header_value != 0;
    for (const unsigned char *p = (const unsigned char *)e->header_value; *p;
         ++p) {
      if (*p < '0' || *p > '9') {
        valid = false;
        break;
      }
      kbps = kbps * 10u + (*p - '0');
      if (kbps > 4096) {
        valid = false;
        break;
      }
    }
    portENTER_CRITICAL(&info_lock);
    if(!s->hls_active || !s->info.bitrate_kbps)s->info.bitrate_kbps = valid && kbps ? kbps : 0;
    portEXIT_CRITICAL(&info_lock);
  } else if (!strcasecmp(e->header_key, "Content-Type")) {
    portENTER_CRITICAL(&info_lock);
    size_t mime_at = 0;
    for (;
         mime_at + 1 < sizeof s->info.content_type && e->header_value[mime_at];
         ++mime_at) {
      unsigned char c = (unsigned char)e->header_value[mime_at];
      s->info.content_type[mime_at] = c >= 32 && c < 127 ? (char)c : '?';
    }
    s->info.content_type[mime_at] = 0;
    portEXIT_CRITICAL(&info_lock);
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
    else if (mime(e->header_value, "video/mp2t") ||
             mime(e->header_value, "audio/mp2t"))
      codec = STREAMOO_CODEC_MPEG_TS;
    else if (mime(e->header_value, "audio/flac") ||
             mime(e->header_value, "audio/x-flac"))
      codec = STREAMOO_CODEC_FLAC;
    else {
      known = false;
      codec = STREAMOO_CODEC_MP3;
      s->playlist_header =
          mime(e->header_value, "audio/x-mpegurl") ||
          mime(e->header_value, "audio/mpegurl") ||
          mime(e->header_value, "application/x-mpegurl") ||
          mime(e->header_value, "application/mpegurl") ||
          mime(e->header_value, "application/vnd.apple.mpegurl") ||
          mime(e->header_value, "audio/x-scpls") ||
          mime(e->header_value, "application/x-scpls") ||
          mime(e->header_value, "application/pls") ||
          mime(e->header_value, "text/plain");
      if (!s->playlist_header && !mime(e->header_value, "audio/ogg") &&
          !mime(e->header_value, "application/ogg") &&
          !mime(e->header_value, "audio/opus") &&
          !mime(e->header_value, "application/octet-stream")) {
        s->bad_header = true;
        s->header_reason = "content_type";
      }
    }
    portENTER_CRITICAL(&info_lock);
    s->response_known = known;
    s->response_codec = codec;
    if (!s->hls_active) {
      s->info.codec_known = known;
      s->info.codec = codec;
    }
    portEXIT_CRITICAL(&info_lock);
  }
  return ESP_OK;
}
/* IDF6.1 get_url reconstructs scheme/host/port/path but omits query.
 * Own the exact accepted request URL, including session/signature parameters.
 */
static esp_err_t set_request_url(streamoo_http_stream_t *s,
                                 esp_http_client_handle_t client,
                                 const char *url) {
  trace(s, NULL, "set_url");
  if (!streamoo_stream_url_allowed(url))
    return fail(s, "url_rejected", ESP_ERR_INVALID_ARG);
  esp_err_t error = esp_http_client_set_url(client, url);
  if (error == ESP_OK)
    memmove(s->request_url, url, strlen(url) + 1);
  return error == ESP_OK ? error : fail(s, "set_url_failed", error);
}
static esp_err_t follow_redirect(streamoo_http_stream_t *s,
                                 esp_http_client_handle_t client) {
  trace(s, NULL, "redirect");
  if (s->bad_location || !s->location[0])
    return fail(s, "redirect_location", ESP_ERR_INVALID_RESPONSE);
  if (!streamoo_url_resolve(s->request_url, s->location, s->redirect_next))
    return fail(s, "redirect_url_rejected", ESP_ERR_INVALID_RESPONSE);
  esp_err_t error = set_request_url(s, client, s->redirect_next);
  if (error == ESP_OK) {
    portENTER_CRITICAL(&info_lock);
    ++s->info.redirects;
    portEXIT_CRITICAL(&info_lock);
  }
  return error;
}
static bool write_audio(streamoo_http_stream_t *s, const uint8_t *data,
                        size_t n) {
  size_t offset = 0;
  int64_t wait_at = 0;
  bool waited = false;
  while (offset < n && !atomic_load(&s->stop)) {
    offset +=
        streamoo_buffered_source_write(s->buffer, 1, data + offset, n - offset);
    if (offset < n) {
      if (!waited) { wait_at = esp_timer_get_time(); waited = true; }
      vTaskDelay(pdMS_TO_TICKS(5) ? pdMS_TO_TICKS(5) : 1);
    }
  }
  if (waited) {
    int64_t now = esp_timer_get_time();
    uint64_t elapsed = now >= wait_at ? (uint64_t)(now - wait_at) : 0;
    portENTER_CRITICAL(&info_lock);
    if (elapsed > s->info.push_wait_max_us) s->info.push_wait_max_us = elapsed;
    portEXIT_CRITICAL(&info_lock);
  }
  return offset == n;
}

/* HLS runs in the HTTP owner task. Documents live outside the compressed ring,
 * because a playlist reload can overlap decoder consumption of queued audio. */
typedef struct {
  char document[STREAMOO_PLAYLIST_LIMIT + 1];
  char url[2049];
  streamoo_hls_manifest_t manifest;
} hls_context_t;
static esp_err_t hls_open(streamoo_http_stream_t *s,
                          esp_http_client_handle_t client, const char *url) {
  esp_http_client_close(client);
  esp_err_t selected = set_request_url(s, client, url);
  if (selected != ESP_OK)
    return selected;
  for (unsigned redirects = 0; redirects <= 5 && !atomic_load(&s->stop);
       ++redirects) {
    response_begin(s);
    esp_err_t error = esp_http_client_open(client, 0);
    if (error != ESP_OK)
      return fail(s, "http_open", error);
    trace(s, NULL, "headers");
    if (esp_http_client_fetch_headers(client) < 0)
      return fail(s, "fetch_headers", ESP_FAIL);
    int code = esp_http_client_get_status_code(client);
    portENTER_CRITICAL(&info_lock);
    s->info.http_status = code;
    portEXIT_CRITICAL(&info_lock);
    if (code == 200) {
      if (s->bad_header)
        return fail(s, s->header_reason ? s->header_reason : "response_headers",
                    ESP_ERR_NOT_SUPPORTED);
      if (s->interval)
        return fail(s, "hls_icy_interval", ESP_ERR_NOT_SUPPORTED);
      trace(s, NULL, "read");
      return ESP_OK;
    }
    if (redirects == 5 || s->bad_location ||
        !(code == 301 || code == 302 || code == 303 || code == 307 ||
          code == 308))
      return fail(s, redirects == 5 ? "redirect_limit" : "http_status",
                  ESP_ERR_INVALID_RESPONSE);
    error = follow_redirect(s, client);
    if (error != ESP_OK)
      return error;
    esp_http_client_close(client);
  }
  return ESP_FAIL;
}
static esp_err_t hls_document(streamoo_http_stream_t *s,
                              esp_http_client_handle_t client, hls_context_t *h,
                              size_t *bytes) {
  *bytes = 0;
  unsigned waits = 0, reads = 0;
  while (!atomic_load(&s->stop)) {
    int n = read_http(s, client, h->document + *bytes,
                      STREAMOO_PLAYLIST_LIMIT + 1 - *bytes);
    if (n == -ESP_ERR_HTTP_EAGAIN && ++waits <= 4)
      continue;
    if (n < 0)
      return fail(s, n == -ESP_ERR_HTTP_EAGAIN ? "read_timeout" : "http_read",
                  n == -ESP_ERR_HTTP_EAGAIN ? ESP_ERR_TIMEOUT : ESP_FAIL);
    if (!n)
      return ESP_OK;
    waits = 0;
    *bytes += (size_t)n;
    if (*bytes > STREAMOO_PLAYLIST_LIMIT)
      return fail(s, "document_limit", ESP_ERR_INVALID_SIZE);
    if ((++reads & 15u) == 0)
      vTaskDelay(1);
  }
  return ESP_FAIL;
}
/* RFC 8216 packed audio starts with ID3. Strip complete, bounded tags before
 * sniffing each segment; never forward transport timestamps into the decoder.
 */
static esp_err_t hls_segment(streamoo_http_stream_t *s,
                             esp_http_client_handle_t client) {
  uint8_t input[1024], prefix[512];
  size_t prefix_bytes = 0, total = 0, skip = 0;
  unsigned waits = 0, reads = 0, tags = 0;
  bool started = false, header_checked = false;
  while (!atomic_load(&s->stop)) {
    int n = read_http(s, client, (char *)input, sizeof input);
    if (n == -ESP_ERR_HTTP_EAGAIN && ++waits <= 4)
      continue;
    if (n < 0)
      return fail(s, n == -ESP_ERR_HTTP_EAGAIN ? "read_timeout" : "http_read",
                  n == -ESP_ERR_HTTP_EAGAIN ? ESP_ERR_TIMEOUT : ESP_FAIL);
    if (!n)
      return started ? ESP_OK
                     : fail(s, "empty_segment", ESP_ERR_INVALID_RESPONSE);
    waits = 0;
    total += (size_t)n;
    if (total > 4u * 1024u * 1024u)
      return fail(s, "segment_limit", ESP_ERR_INVALID_SIZE);
    if (!started)
      trace(s, NULL, "sniff");
    for (size_t at = 0; at < (size_t)n;) {
      if (skip) {
        size_t take = (size_t)n - at;
        if (take > skip)
          take = skip;
        skip -= take;
        at += take;
        continue;
      }
      if (started) {
        if (!write_audio(s, input + at, (size_t)n - at))
          return ESP_FAIL;
        break;
      }
      prefix[prefix_bytes++] = input[at++];
      if (!header_checked && prefix_bytes == 10) {
        if (!memcmp(prefix, "ID3", 3)) {
          if (++tags > 4 || prefix[3] < 2 || prefix[3] > 4 ||
              ((prefix[6] | prefix[7] | prefix[8] | prefix[9]) & 0x80))
            return fail(s, "id3_header", ESP_ERR_INVALID_RESPONSE);
          skip = ((size_t)prefix[6] << 21) | ((size_t)prefix[7] << 14) |
                 ((size_t)prefix[8] << 7) | prefix[9];
          if (prefix[3] == 4 && (prefix[5] & 0x10))
            skip += 10;
          if (skip > 16384)
            return fail(s, "id3_limit", ESP_ERR_INVALID_SIZE);
          prefix_bytes = 0;
          continue;
        }
        header_checked = true;
      }
      if (header_checked && prefix_bytes >= 10) {
        streamoo_codec_t codec = STREAMOO_CODEC_MP3;
        bool known = false;
        if (prefix[0] == 0xff && (prefix[1] & 0xf6) == 0xf0) {
          codec = STREAMOO_CODEC_AAC_LC;
          known = true;
        } else if (prefix[0] == 0xff && (prefix[1] & 0xe0) == 0xe0) {
          codec = STREAMOO_CODEC_MP3;
          known = true;
        } else if (prefix_bytes >= 377 && prefix[0] == 0x47 &&
                   prefix[188] == 0x47 && prefix[376] == 0x47) {
          codec = STREAMOO_CODEC_MPEG_TS;
          known = true;
        }
        if (known) {
          portENTER_CRITICAL(&info_lock);
          bool changed = s->info.codec_known && s->info.codec != codec;
          if (!changed) {
            s->info.codec = codec;
            s->info.codec_known = true;
          }
          portEXIT_CRITICAL(&info_lock);
          if (changed)
            return fail(s, "codec_change", ESP_ERR_NOT_SUPPORTED);
          if (!write_audio(s, prefix, prefix_bytes))
            return ESP_FAIL;
          started = true;
        } else if (prefix_bytes == sizeof prefix)
          return fail(s, "codec_probe", ESP_ERR_NOT_SUPPORTED);
      }
    }
    trace(s, NULL, "read");
    if ((++reads & 15u) == 0)
      vTaskDelay(1);
  }
  return ESP_FAIL;
}
static esp_err_t hls_run(streamoo_http_stream_t *s,
                         esp_http_client_handle_t client,
                         const uint8_t *document, size_t bytes,
                         const char *url) {
  portENTER_CRITICAL(&info_lock);
  s->info.hls = true;
  portEXIT_CRITICAL(&info_lock);
  trace(s, "hls_manifest", "alloc");
  hls_context_t *h =
      heap_caps_calloc(1, sizeof *h, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!h)
    return fail(s, "manifest_alloc", ESP_ERR_NO_MEM);
  memcpy(h->document, document, bytes);
  strcpy(h->url, url);
  s->hls_active = true;
  trace(s, "hls_manifest", "parse");
  esp_http_client_set_header(client, "Icy-MetaData", "0");
  portENTER_CRITICAL(&info_lock);
  s->info.codec_known = false;
  s->info.bitrate_kbps = 0;
  portEXIT_CRITICAL(&info_lock);
  esp_err_t error = ESP_OK;
  unsigned depth = 0, unchanged = 0;
  uint64_t next = 0;
  bool initial = true, played = false;
  for (; error == ESP_OK && !atomic_load(&s->stop);) {
    trace(s, NULL, "parse");
    streamoo_hls_result_t result =
        streamoo_hls_parse(h->document, bytes, h->url, &h->manifest);
    if (result == STREAMOO_HLS_MASTER) {
      if (played || ++depth > 3) {
        error = fail(s, played ? "master_after_media" : "master_limit",
                     ESP_ERR_NOT_SUPPORTED);
        break;
      }
      /* Media reloads and segment headers do not redefine the variant's
       * declaration. A newly selected nested master can replace or clear it. */
      portENTER_CRITICAL(&info_lock);
      s->info.bitrate_kbps=(h->manifest.bitrate_bps+500u)/1000u;
      portEXIT_CRITICAL(&info_lock);
      trace(s, "hls_manifest", NULL);
      error = hls_open(s, client, h->manifest.variant);
      if (error == ESP_OK)
        error = hls_document(s, client, h, &bytes);
      if (error == ESP_OK)
        strcpy(h->url, s->request_url);
      continue;
    }
    if (result != STREAMOO_HLS_MEDIA) {
      error =
          fail(s,
               result == STREAMOO_HLS_UNSUPPORTED ? "manifest_feature"
                                                  : "manifest_invalid",
               result == STREAMOO_HLS_UNSUPPORTED ? ESP_ERR_NOT_SUPPORTED
                                                  : ESP_ERR_INVALID_RESPONSE);
      break;
    }
    streamoo_hls_manifest_t *m = &h->manifest;
    if (initial) {
      next = m->sequence + (m->endlist || m->count <= 3 ? 0 : m->count - 3);
      initial = false;
    }
    portENTER_CRITICAL(&info_lock);
    s->info.hls_media_sequence = m->sequence;
    s->info.hls_media_count = m->count;
    s->info.hls_next = next;
    portEXIT_CRITICAL(&info_lock);
    if (next < m->sequence) {
      error = fail(s, "media_window_missed", ESP_ERR_INVALID_RESPONSE);
      break;
    }
    unsigned fetched = 0;
    for (unsigned i = 0;
         i < m->count && error == ESP_OK && !atomic_load(&s->stop); ++i) {
      uint64_t sequence = m->sequence + i;
      if (sequence < next)
        continue;
      trace(s, "hls_segment", "sequence");
      portENTER_CRITICAL(&info_lock);
      s->info.hls_sequence = sequence;
      portEXIT_CRITICAL(&info_lock);
      if (sequence != next || (played && m->segments[i].discontinuity)) {
        error = fail(s, sequence != next ? "segment_sequence" : "discontinuity",
                     ESP_ERR_NOT_SUPPORTED);
        break;
      }
      error = hls_open(s, client, m->segments[i].url);
      if (error == ESP_OK)
        error = hls_segment(s, client);
      if (error == ESP_OK) {
        if (next == UINT64_MAX) {
          error = fail(s, "sequence_overflow", ESP_ERR_INVALID_RESPONSE);
          break;
        }
        ++next;
        portENTER_CRITICAL(&info_lock);
        ++s->info.hls_segments;
        s->info.hls_next = next;
        portEXIT_CRITICAL(&info_lock);
        ++fetched;
        played = true;
      }
    }
    if (error != ESP_OK)
      break;
    if (m->endlist) {
      error = fail(s, "endlist", ESP_ERR_INVALID_RESPONSE);
      break;
    }
    if (fetched)
      unchanged = 0;
    else if (++unchanged >= 6) {
      error = fail(s, "manifest_stalled", ESP_ERR_TIMEOUT);
      break;
    }
    /* Half target duration, interruptible within 100 ms. Backpressure in the
     * compressed ring naturally paces downloading without dropping segments. */
    trace(s, NULL, "reload_wait");
    for (unsigned ms = 0; ms < m->target_s * 500u && !atomic_load(&s->stop);
         ms += 100)
      vTaskDelay(pdMS_TO_TICKS(100) ? pdMS_TO_TICKS(100) : 1);
    if (atomic_load(&s->stop))
      break;
    trace(s, "hls_reload", NULL);
    portENTER_CRITICAL(&info_lock);
    ++s->info.hls_reloads;
    portEXIT_CRITICAL(&info_lock);
    error = hls_open(s, client, h->url);
    if (error == ESP_OK)
      error = hls_document(s, client, h, &bytes);
    if (error == ESP_OK)
      strcpy(h->url, s->request_url);
  }
  heap_caps_free(h);
  return error;
}
static void reader(void *context) {
  streamoo_http_stream_t *s = context;
#ifdef ESP_PLATFORM
  ESP_LOGI("http_reader", "Task placement: HTTP/Wi-Fi core=%d (affinity=0)",
           xPortGetCoreID());
#endif
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
  trace(s, "stream", "client_init");
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t error = client ? ESP_OK : ESP_ERR_NO_MEM;
  if (!client)
    fail(s, "client_alloc", ESP_ERR_NO_MEM);
  bool accepted = false;
  if (client) {
    esp_http_client_set_header(client, "Icy-MetaData", "1");
    esp_http_client_set_header(client, "Accept-Encoding", "identity");
  }
  unsigned redirects = 0, playlists = 0;
  for (unsigned hop = 0; error == ESP_OK && !atomic_load(&s->stop) && hop <= 8;
       ++hop) {
    response_begin(s);
    portENTER_CRITICAL(&info_lock);
    s->info.codec_known = false;
    s->info.bitrate_kbps = 0;
    portEXIT_CRITICAL(&info_lock);
    error = esp_http_client_open(client, 0);
    if (error != ESP_OK) {
      fail(s, "http_open", error);
      break;
    }
    trace(s, NULL, "headers");
    if (esp_http_client_fetch_headers(client) < 0) {
      error = fail(s, "fetch_headers", ESP_FAIL);
      break;
    }
    int code = esp_http_client_get_status_code(client);
    portENTER_CRITICAL(&info_lock);
    s->info.http_status = code;
    portEXIT_CRITICAL(&info_lock);
    if (code == 200) {
      if (s->bad_header) {
        error =
            fail(s, s->header_reason ? s->header_reason : "response_headers",
                 ESP_ERR_NOT_SUPPORTED);
        break;
      }
      strcpy(s->redirect_old, s->request_url);
      streamoo_http_stream_info_t snapshot;
      streamoo_http_stream_info(s, &snapshot);
      if (s->playlist_header ||
          (!snapshot.codec_known && streamoo_playlist_url(s->redirect_old))) {
        if (++playlists > 3 || s->interval) {
          error = fail(s, "playlist_limit_or_icy", ESP_ERR_NOT_SUPPORTED);
          break;
        }
        trace(s, "playlist", "read");
        /* The compressed ring is still empty. Reuse its PSRAM as document
         * staging. */
        size_t bytes = 0;
        unsigned waits = 0, document_reads = 0;
        for (;;) {
          if (atomic_load(&s->stop)) {
            error = ESP_FAIL;
            break;
          }
          int n = read_http(s, client, (char *)s->storage + bytes,
                            STREAMOO_PLAYLIST_LIMIT + 1 - bytes);
          if (n == -ESP_ERR_HTTP_EAGAIN && ++waits <= 4)
            continue;
          if (n < 0) {
            error = fail(
                s, n == -ESP_ERR_HTTP_EAGAIN ? "read_timeout" : "http_read",
                ESP_FAIL);
            break;
          }
          if (!n)
            break;
          waits = 0;
          bytes += (size_t)n;
          if ((++document_reads & 15u) == 0)
            vTaskDelay(1);
          if (bytes > STREAMOO_PLAYLIST_LIMIT) {
            error = fail(s, "document_limit", ESP_ERR_INVALID_SIZE);
            break;
          }
        }
        if (error != ESP_OK)
          break;
        if (streamoo_hls_detect(s->storage, bytes)) {
          error = hls_run(s, client, s->storage, bytes, s->redirect_old);
          goto finished;
        }
        trace(s, NULL, "parse");
        streamoo_playlist_result_t result = streamoo_playlist_resolve(
            (char *)s->storage, bytes, s->redirect_old, s->redirect_next);
        if (result != STREAMOO_PLAYLIST_OK) {
          error =
              fail(s, "playlist_invalid",
                   result == STREAMOO_PLAYLIST_HLS ? ESP_ERR_NOT_SUPPORTED
                                                   : ESP_ERR_INVALID_RESPONSE);
          break;
        }
        error = set_request_url(s, client, s->redirect_next);
        if (error != ESP_OK)
          break;
        esp_http_client_close(client);
        continue;
      }
      trace(s, "stream", "read");
      accepted = true;
      break;
    }
    if (!(code == 301 || code == 302 || code == 303 || code == 307 ||
          code == 308) ||
        redirects++ == 5) {
      error = fail(s, redirects > 5 ? "redirect_limit" : "http_status",
                   ESP_ERR_INVALID_RESPONSE);
      break;
    }
    error = follow_redirect(s, client);
    if (error != ESP_OK)
      break;
    esp_http_client_close(client);
  }
  if (error == ESP_OK &&
      (!accepted || s->bad_header || !streamoo_icy_init(&s->icy, s->interval)))
    error = ESP_ERR_INVALID_RESPONSE;
  uint8_t input[1024], audio[1024];
  size_t total = 0;
  unsigned timeouts = 0, reads = 0;
  while (error == ESP_OK && !atomic_load(&s->stop)) {
    int n = read_http(s, client, (char *)input, sizeof input);
    if (n == -ESP_ERR_HTTP_EAGAIN) {
      if (++timeouts <= 4)
        continue;
      error = fail(s, "read_timeout", ESP_ERR_TIMEOUT);
      break;
    }
    timeouts = 0;
    if (n <= 0) {
      error = fail(s, n < 0 ? "http_read" : "http_eof",
                   n < 0 ? ESP_FAIL : ESP_ERR_INVALID_RESPONSE);
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
      error = fail(s, "icy_frame", ESP_ERR_INVALID_RESPONSE);
      break;
    }
    if (bytes) {
      portENTER_CRITICAL(&info_lock);
      if (!s->info.codec_known) {
        size_t take = sizeof s->probe - s->probe_bytes;
        if (take > bytes)
          take = bytes;
        memcpy(s->probe + s->probe_bytes, audio, take);
        s->probe_bytes += take;
      }
      if (!s->info.codec_known && s->probe_bytes >= 3) {
        if (s->probe[0] == 0xff && (s->probe[1] & 0xf6) == 0xf0) {
          s->info.codec = STREAMOO_CODEC_AAC_LC;
          s->info.codec_known = true;
        } else if (!memcmp(s->probe, "ID3", 3) ||
                   (s->probe[0] == 0xff && (s->probe[1] & 0xe0) == 0xe0)) {
          s->info.codec = STREAMOO_CODEC_MP3;
          s->info.codec_known = true;
        } else if (s->probe_bytes >= 4 && !memcmp(s->probe, "fLaC", 4)) {
          s->info.codec = STREAMOO_CODEC_FLAC;
          s->info.codec_known = true;
        } else if (s->probe_bytes >= 27 && !memcmp(s->probe, "OggS", 4) &&
                   s->probe[4] == 0 && (s->probe[5] & 2)) {
          size_t payload = 27u + s->probe[26];
          if (s->probe_bytes >= payload + 8 &&
              !memcmp(s->probe + payload, "OpusHead", 8)) {
            s->info.codec = STREAMOO_CODEC_OPUS;
            s->info.codec_known = true;
          } else if (s->probe_bytes >= payload + 7 &&
                     !memcmp(s->probe + payload, "\1vorbis", 7)) {
            s->info.codec = STREAMOO_CODEC_VORBIS;
            s->info.codec_known = true;
          }
        }
      }
      bool known = s->info.codec_known;
      portEXIT_CRITICAL(&info_lock);
      total += bytes;
      if (!known && total > 32768) {
        error = fail(s, "codec_probe_limit", ESP_ERR_NOT_SUPPORTED);
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
    /* Also yield on metadata-heavy or very fast sources with no backpressure.
     */
    if ((reads & 15u) == 0)
      vTaskDelay(1);
  }
finished:
  if (atomic_load(&s->stop))
    fail(s, "cancelled", error);
  else if (error != ESP_OK)
    fail(s, "reader_error", error);
  if (client) {
    int socket_error = esp_http_client_get_errno(client), tls_code = 0,
        tls_flags = 0;
    esp_err_t tls_error = ESP_OK;
    if (!strncmp(s->request_url, "https://", 8))
      tls_error = esp_http_client_get_and_clear_last_tls_error(
          client, &tls_code, &tls_flags);
    portENTER_CRITICAL(&info_lock);
    s->info.socket_errno = socket_error;
    s->info.tls_code = tls_code;
    s->info.tls_flags = tls_flags;
    s->info.tls_error = tls_error;
    portEXIT_CRITICAL(&info_lock);
    esp_http_client_cleanup(client);
  }
  portENTER_CRITICAL(&info_lock);
  s->info.ended = true;
  s->info.error = error;
  portEXIT_CRITICAL(&info_lock);
  /* Publish terminal diagnostics before readers can observe EOF. Keep queued
   * bytes drainable; source_eof alone is not the original HTTP/HLS cause. */
  while (!atomic_load(&s->stop) &&
         !streamoo_buffered_source_finish(s->buffer, 1, false))
    vTaskDelay(1);
  xSemaphoreGive(s->done);
  vTaskDelete(NULL);
}
esp_err_t streamoo_http_stream_open(const char *url,
                                    streamoo_http_stream_t **out) {
  if (!out || !url || !streamoo_stream_url_allowed(url))
    return ESP_ERR_INVALID_ARG;
  *out = NULL;
  streamoo_http_stream_t *s =
      heap_caps_calloc(1, sizeof *s, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!s)
    return ESP_ERR_NO_MEM;
  atomic_init(&s->stop, 0);
  strcpy(s->url, url);
  strcpy(s->request_url, url);
  s->storage = heap_caps_malloc(CAPACITY, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  s->done = xSemaphoreCreateBinary();
  if (s->storage)
    s->buffer = streamoo_buffered_source_create(s->storage, CAPACITY);
  if (!s->done || !s->buffer) {
    if (s->done)
      vSemaphoreDelete(s->done);
    streamoo_buffered_source_destroy(s->buffer);
    heap_caps_free(s->storage);
    heap_caps_free(s);
    return ESP_ERR_NO_MEM;
  }
#ifdef ESP_PLATFORM
  int reader_created = xTaskCreatePinnedToCore(reader, "http_reader", 16384,
                                                 s, 3, NULL, 0);
#else
  int reader_created = xTaskCreate(reader, "http_reader", 16384, s, 3, NULL);
#endif
  if (reader_created != pdPASS) {
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
  if(result==STREAMOO_SOURCE_WAIT) {
    uint32_t empty=0,busy=0;
    streamoo_buffered_source_waits(s->buffer,&empty,&busy);
    portENTER_CRITICAL(&info_lock);
    s->info.source_empty_waits=empty;
    s->info.source_busy_waits=busy;
    portEXIT_CRITICAL(&info_lock);
  }
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
