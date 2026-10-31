#include "streamoo_live_decode.h"
#include <string.h>
bool streamoo_live_decode_init(streamoo_live_decode_t *s,
                               streamoo_byte_source_t source, void *context,
                               streamoo_stream_decode_fn decode) {
  if (!s || !source.read || !decode)
    return false;
  *s = (streamoo_live_decode_t){
      .source = source, .context = context, .decode = decode};
  return true;
}
static streamoo_stream_result_t packet(streamoo_live_decode_t *s) {
  for (unsigned work = 0; work < 256; ++work) {
    if (!s->pending) {
      if (s->eof)
        return STREAMOO_STREAM_FAILED;
      size_t n = 0;
      bool last = false;
      streamoo_source_result_t r = s->source.read(s->source.context, s->input,
                                                  sizeof s->input, &n, &last);
      if (r == STREAMOO_SOURCE_WAIT)
        return STREAMOO_STREAM_WAIT;
      if (r != STREAMOO_SOURCE_DATA || !n || n > sizeof s->input)
        return STREAMOO_STREAM_FAILED;
      s->pending = n;
      s->input_cursor = 0;
      s->eof = last;
    }
    streamoo_decode_result_t r = {0};
    esp_err_t error = s->decode(s->context, s->input + s->input_cursor,
                                s->pending, s->eof, s->pcm, sizeof s->pcm, &r);
    s->decoder_error = error;
    s->codec_status = r.codec_status;
    if (r.produced)
      s->observed = r.format;
    if (error != ESP_OK || r.consumed > s->pending ||
        r.produced > sizeof s->pcm || (!r.consumed && !r.produced))
      return STREAMOO_STREAM_FAILED;
    s->pending -= r.consumed;
    s->input_cursor += r.consumed;
    if (r.produced) {
      if (r.format.bits != 16 || !r.format.channels || r.format.channels > 2 ||
          r.format.rate < 8000 || r.format.rate > 48000 ||
          r.produced % (2 * r.format.channels))
        return STREAMOO_STREAM_FAILED;
      if (s->format.rate && (s->format.rate != r.format.rate ||
                             s->format.channels != r.format.channels))
        return STREAMOO_STREAM_FAILED;
      s->format = r.format;
      s->pcm_bytes = r.produced;
      s->pcm_cursor = 0;
      return STREAMOO_STREAM_READY;
    }
  }
  return STREAMOO_STREAM_WAIT;
}
static streamoo_stream_result_t sample(streamoo_live_decode_t *s,
                                       int32_t out[2]) {
  if (s->pcm_cursor == s->pcm_bytes) {
    streamoo_stream_result_t r = packet(s);
    if (r != STREAMOO_STREAM_READY)
      return r;
  }
  const uint8_t *p = s->pcm + s->pcm_cursor;
  out[0] = (int16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
  out[1] = s->format.channels == 1
               ? out[0]
               : (int16_t)((uint16_t)p[2] | (uint16_t)p[3] << 8);
  s->pcm_cursor += 2 * s->format.channels;
  return STREAMOO_STREAM_READY;
}
streamoo_stream_result_t streamoo_live_decode_read(streamoo_live_decode_t *s,
                                                   int32_t *out) {
  if (!s || !out || !s->decode || s->failed)
    return STREAMOO_STREAM_FAILED;
  streamoo_stream_result_t r;
  while (s->assembled < 240) {
    if (!s->have_a) {
      r = sample(s, s->a);
      if (r != STREAMOO_STREAM_READY)
        goto unavailable;
      s->have_a = true;
    }
    if (s->advance) {
      memcpy(s->a, s->b, sizeof s->a);
      s->have_b = false;
      s->advance = false;
    }
    if (!s->have_b) {
      r = sample(s, s->b);
      if (r != STREAMOO_STREAM_READY)
        goto unavailable;
      s->have_b = true;
    }
    for (unsigned c = 0; c < 2; ++c) {
      int64_t value =
          (int64_t)s->a[c] * 48000 + (int64_t)(s->b[c] - s->a[c]) * s->phase;
      s->block[s->assembled * 2 + c] = (int32_t)(value * 65536 / 48000);
    }
    ++s->assembled;
    s->phase += s->format.rate;
    if (s->phase >= 48000) {
      s->phase -= 48000;
      s->advance = true;
    }
  }
  memcpy(out, s->block, sizeof s->block);
  s->assembled = 0;
  return STREAMOO_STREAM_READY;
unavailable:
  if (r == STREAMOO_STREAM_FAILED)
    s->failed = true;
  return r;
}
