#pragma once
#include "streamoo_buffered_source.h"
#include "streamoo_decoder.h"
typedef struct streamoo_http_stream streamoo_http_stream_t;
typedef struct {
  bool codec_known, ended;
  streamoo_codec_t codec;
  esp_err_t error;
  int http_status;
  uint32_t reader_stack_hwm;
  uint32_t
      bitrate_kbps; /* Server-declared icy-br,0=unknown; not measured VBR. */
  uint32_t metadata_revision;
  char title[257];
  /* Reader-owned diagnostic snapshot; no URLs, headers with secrets or
   * pointers. */
  char resource[20], stage[24], reason[40], content_type[80];
  int last_read, socket_errno, tls_code, tls_flags;
  esp_err_t tls_error;
  uint64_t response_bytes, received_bytes, read_calls;
  bool hls;
  uint64_t hls_sequence, hls_next, hls_media_sequence;
  uint32_t redirects, hls_segments, hls_reloads, hls_media_count;
} streamoo_http_stream_info_t;
esp_err_t streamoo_http_stream_open(const char *url, streamoo_http_stream_t **);
bool streamoo_http_stream_info(streamoo_http_stream_t *,
                               streamoo_http_stream_info_t *);
streamoo_byte_source_t streamoo_http_stream_source(streamoo_http_stream_t *);
size_t streamoo_http_stream_queued(streamoo_http_stream_t *);
/* Async cancellation; dispose returns false until reader ACK. Never free live
 * IO. */
void streamoo_http_stream_stop(streamoo_http_stream_t *);
bool streamoo_http_stream_dispose(streamoo_http_stream_t *);
