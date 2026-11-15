#include "streamoo_icy.h"
#include <string.h>
bool streamoo_icy_init(streamoo_icy_t *s, uint32_t interval) {
  if (!s || interval > 16777216u)
    return false;
  *s = (streamoo_icy_t){.interval = interval, .audio_left = interval};
  return true;
}
static void title(streamoo_icy_t *s) {
  const char key[] = "StreamTitle='";
  for (size_t i = 0; i + sizeof key - 1 <= s->used; ++i)
    if (!memcmp(s->metadata + i, key, sizeof key - 1)) {
      size_t begin = i + sizeof key - 1, end = begin;
      while (end < s->used && s->metadata[end] &&
             !(s->metadata[end] == '\'' && end + 1 < s->used &&
               s->metadata[end + 1] == ';'))
        ++end;
      if (end == s->used || !s->metadata[end])
        return;
      char text[257] = {0};
      size_t n = end - begin;
      if (n > 256)
        n = 256;
      if (n < end - begin)
        while (n && ((s->metadata[begin + n] & 0xc0) == 0x80))
          --n;
      for (size_t j = 0; j < n; ++j) {
        uint8_t c = s->metadata[begin + j];
        text[j] = (c < 32 || c == 127) ? ' ' : (char)c;
      }
      if (strcmp(s->title, text)) {
        memcpy(s->title, text, sizeof text);
        ++s->revision;
        s->title_audio_offset = s->audio_total;
      }
      return;
    }
}
bool streamoo_icy_feed(streamoo_icy_t *s, const uint8_t *in, size_t n,
                       uint8_t *out, size_t cap, size_t *written) {
  if (!s || (!in && n) || !out || !written || cap < n)
    return false;
  *written = 0;
  if (!n)
    return true;
  if (!s->interval) {
    memcpy(out, in, n);
    *written = n;
    s->audio_total += n;
    return true;
  }
  for (size_t i = 0; i < n;) {
    if (s->metadata_left) {
      size_t take = n - i;
      if (take > s->metadata_left)
        take = s->metadata_left;
      memcpy(s->metadata + s->used, in + i, take);
      s->used += take;
      s->metadata_left -= take;
      i += take;
      if (!s->metadata_left) {
        s->metadata[s->used] = 0;
        title(s);
        s->audio_left = s->interval;
      }
    } else if (s->audio_left) {
      size_t take = n - i;
      if (take > s->audio_left)
        take = s->audio_left;
      memcpy(out + *written, in + i, take);
      *written += take;
      s->audio_total += take;
      i += take;
      s->audio_left -= take;
    } else {
      s->metadata_left = (uint32_t)in[i++] * 16;
      s->used = 0;
      if (!s->metadata_left)
        s->audio_left = s->interval;
    }
  }
  return true;
}
