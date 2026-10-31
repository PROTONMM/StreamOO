#pragma once
#include "streamoo_decode_stream.h"
/* Decoder task only. 16-bit mono/stereo 8..48kHz -> 48kHz stereo32.
 * Linear interpolation, rational phase retained across packets and WAIT.
 * Live EOF fails to owner; never rewind/replay a network source. */
typedef enum {
  STREAMOO_LIVE_FAILURE_NONE,
  STREAMOO_LIVE_SOURCE_EOF,
  STREAMOO_LIVE_SOURCE_ERROR,
  STREAMOO_LIVE_SOURCE_CONTRACT,
  STREAMOO_LIVE_DECODER_ERROR,
  STREAMOO_LIVE_DECODER_BOUNDS,
  STREAMOO_LIVE_DECODER_NO_PROGRESS,
  STREAMOO_LIVE_PCM_FORMAT,
  STREAMOO_LIVE_FORMAT_CHANGED
} streamoo_live_failure_t;
typedef struct {
  streamoo_byte_source_t source;
  void *context;
  streamoo_stream_decode_fn decode;
  uint8_t input[257];
  _Alignas(int32_t) uint8_t pcm[8192];
  size_t pending, input_cursor, pcm_bytes, pcm_cursor, assembled;
  streamoo_pcm_format_t format, observed;
  int32_t codec_status;
  esp_err_t decoder_error;
  streamoo_live_failure_t failure;
  uint64_t source_calls, decode_calls, fetched_total, consumed_total,
      produced_total;
  size_t last_input, last_consumed, last_produced, last_needed;
  int32_t block[480], a[2], b[2];
  uint32_t phase;
  bool have_a, have_b, advance, eof, failed;
} streamoo_live_decode_t;
bool streamoo_live_decode_init(streamoo_live_decode_t *, streamoo_byte_source_t,
                               void *, streamoo_stream_decode_fn);
streamoo_stream_result_t streamoo_live_decode_read(streamoo_live_decode_t *,
                                                   int32_t *);
/* Stable, non-private diagnostic tokens; first terminal cause stays latched
 * until init. Counters are per session and never touched from the I2S ISR. */
const char *streamoo_live_failure_name(streamoo_live_failure_t);
