#pragma once
#include "streamoo_decode_stream.h"
/* Decoder task only. 16-bit mono/stereo 8..48kHz -> 48kHz stereo32.
 * Linear interpolation, rational phase retained across packets and WAIT.
 * Live EOF fails to owner; never rewind/replay a network source. */
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
  int32_t block[480], a[2], b[2];
  uint32_t phase;
  bool have_a, have_b, advance, eof, failed;
} streamoo_live_decode_t;
bool streamoo_live_decode_init(streamoo_live_decode_t *, streamoo_byte_source_t,
                               void *, streamoo_stream_decode_fn);
streamoo_stream_result_t streamoo_live_decode_read(streamoo_live_decode_t *,
                                                   int32_t *);
