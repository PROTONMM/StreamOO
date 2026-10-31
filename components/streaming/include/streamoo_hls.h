#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_HLS_SEGMENTS 32u
typedef struct {
  char url[2049];
  bool discontinuity;
} streamoo_hls_segment_t;
typedef struct {
  uint64_t sequence;
  uint32_t target_s, count;
  bool endlist;
  char variant[2049];
  streamoo_hls_segment_t segments[STREAMOO_HLS_SEGMENTS];
  uint32_t
      bitrate_bps; /* Selected variant declaration;0 when not attributable. */
} streamoo_hls_manifest_t;
typedef enum {
  STREAMOO_HLS_MEDIA,
  STREAMOO_HLS_MASTER,
  STREAMOO_HLS_INVALID,
  STREAMOO_HLS_UNSUPPORTED
} streamoo_hls_result_t;
bool streamoo_hls_detect(const uint8_t *, size_t);
/* Bounded original RFC8216 parser. Mutable document needs bytes+1 storage. */
streamoo_hls_result_t streamoo_hls_parse(char *, size_t, const char *,
                                         streamoo_hls_manifest_t *);
