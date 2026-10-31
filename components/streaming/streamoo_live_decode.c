#include "streamoo_live_decode.h"
#include <string.h>
const char *streamoo_live_failure_name(streamoo_live_failure_t reason) {
  switch (reason) {
  case STREAMOO_LIVE_FAILURE_NONE:
    return "none";
  case STREAMOO_LIVE_SOURCE_EOF:
    return "source_eof";
  case STREAMOO_LIVE_SOURCE_ERROR:
    return "source_error";
  case STREAMOO_LIVE_SOURCE_CONTRACT:
    return "source_contract";
  case STREAMOO_LIVE_DECODER_ERROR:
    return "decoder_error";
  case STREAMOO_LIVE_DECODER_BOUNDS:
    return "decoder_bounds";
  case STREAMOO_LIVE_DECODER_NO_PROGRESS:
    return "decoder_no_progress";
  case STREAMOO_LIVE_PCM_FORMAT:
    return "pcm_format";
  case STREAMOO_LIVE_FORMAT_CHANGED:
    return "format_changed";
  default:
    return "unknown";
  }
}
static streamoo_stream_result_t failure(streamoo_live_decode_t *s,
                                        streamoo_live_failure_t reason) {
  if (s->failure == STREAMOO_LIVE_FAILURE_NONE)
    s->failure = reason;
  return STREAMOO_STREAM_FAILED;
}
bool streamoo_live_decode_init(streamoo_live_decode_t *s,
                               streamoo_byte_source_t source, void *context,
                               streamoo_stream_decode_fn decode) {
  if (!s || !source.read || !decode)
    return false;
  memset(s,0,sizeof *s);
  s->source=source;s->context=context;s->decode=decode;
  return true;
}
static streamoo_stream_result_t packet(streamoo_live_decode_t *s) {
  for (unsigned work = 0; work < 256; ++work) {
    if (!s->pending) {
      if (s->eof)
        return failure(s, STREAMOO_LIVE_SOURCE_EOF);
      size_t n = 0;
      bool last = false;
      ++s->source_calls;
      streamoo_source_result_t r = s->source.read(s->source.context, s->input,
                                                  sizeof s->input, &n, &last);
      if (r != STREAMOO_SOURCE_DATA && (n || last))
        return failure(s, STREAMOO_LIVE_SOURCE_CONTRACT);
      if (r == STREAMOO_SOURCE_WAIT)
        return STREAMOO_STREAM_WAIT;
      if (r == STREAMOO_SOURCE_END)
        return failure(s, STREAMOO_LIVE_SOURCE_EOF);
      if (r == STREAMOO_SOURCE_ERROR)
        return failure(s, STREAMOO_LIVE_SOURCE_ERROR);
      if (r != STREAMOO_SOURCE_DATA || !n || n > sizeof s->input)
        return failure(s, STREAMOO_LIVE_SOURCE_CONTRACT);
      s->fetched_total += n;
      s->pending = n;
      s->input_cursor = 0;
      s->eof = last;
    }
    streamoo_decode_result_t r = {0};
    ++s->decode_calls;
    s->last_input = s->pending;
    esp_err_t error = s->decode(s->context, s->input + s->input_cursor,
                                s->pending, s->eof, s->pcm, sizeof s->pcm, &r);
    s->decoder_error = error;
    s->codec_status = r.codec_status;
    s->last_consumed = r.consumed;
    s->last_produced = r.produced;
    s->last_needed = r.needed;
    if (r.produced)
      s->observed = r.format;
    if (error != ESP_OK)
      return failure(s, STREAMOO_LIVE_DECODER_ERROR);
    if (r.consumed > s->pending || r.produced > sizeof s->pcm)
      return failure(s, STREAMOO_LIVE_DECODER_BOUNDS);
    if (!r.consumed && !r.produced)
      return failure(s, STREAMOO_LIVE_DECODER_NO_PROGRESS);
    s->consumed_total += r.consumed;
    s->produced_total += r.produced;
    s->pending -= r.consumed;
    s->input_cursor += r.consumed;
    if (r.produced) {
      if ((r.format.bits != 16 && r.format.bits != 24 && r.format.bits != 32) || !r.format.channels || r.format.channels > 2 ||
          r.format.rate < 8000 || r.format.rate > 48000 ||
          r.produced % ((r.format.bits/8) * r.format.channels))
        return failure(s, STREAMOO_LIVE_PCM_FORMAT);
      if (s->format.rate && (s->format.rate != r.format.rate ||
                             s->format.channels != r.format.channels || s->format.bits != r.format.bits))
        return failure(s, STREAMOO_LIVE_FORMAT_CHANGED);
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
  unsigned width=s->format.bits/8;
  for(unsigned c=0;c<2;++c){
    const uint8_t *q=p+(s->format.channels==1 ? 0:c*width);
    uint32_t word=0;
    for(unsigned j=0;j<width;++j)word|=(uint32_t)q[j]<<(8*j);
    int64_t value=(int64_t)word;
    if(word & (UINT32_C(1)<<(s->format.bits-1)))value-=INT64_C(1)<<s->format.bits;
    out[c]=(int32_t)(value*(INT64_C(1)<<(32-s->format.bits)));
  }
  s->pcm_cursor += width * s->format.channels;
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
          (int64_t)s->a[c] * 48000 + ((int64_t)s->b[c] - s->a[c]) * s->phase;
      s->block[s->assembled * 2 + c] = (int32_t)(value / 48000);
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
