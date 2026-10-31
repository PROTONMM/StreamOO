#include "streamoo_hls.h"
#include "streamoo_playlist_resolver.h"
#include <limits.h>
#include <string.h>
static bool number(const char *p, uint64_t *out) {
  if (!*p)
    return false;
  uint64_t n = 0;
  for (; *p; ++p) {
    if (*p < '0' || *p > '9' || n > (UINT64_MAX - (unsigned)(*p - '0')) / 10)
      return false;
    n = n * 10 + (unsigned)(*p - '0');
  }
  *out = n;
  return true;
}
static bool attribute(const char *p, const char *key, char *out,
                      size_t capacity) {
  while (*p) {
    const char *name = p;
    while (*p && *p != '=' && *p != ',')
      ++p;
    if (*p != '=')
      return false;
    size_t keylen = (size_t)(p - name);
    ++p;
    bool quoted = *p == '"';
    if (quoted)
      ++p;
    const char *value = p;
    while (*p && (quoted ? *p != '"' : *p != ','))
      ++p;
    size_t n = (size_t)(p - value);
    if (quoted) {
      if (*p != '"')
        return false;
      ++p;
      if (*p && *p != ',')
        return false;
    }
    if (strlen(key) == keylen && !memcmp(name, key, keylen)) {
      if (n >= capacity)
        return false;
      memcpy(out, value, n);
      out[n] = 0;
      return true;
    }
    if (*p == ',')
      ++p;
  }
  return false;
}
bool streamoo_hls_detect(const uint8_t *p, size_t n) {
  for (size_t i = 0; i + 7 <= n; ++i)
    if ((!i || p[i - 1] == '\n' || p[i - 1] == '\r') &&
        !memcmp(p + i, "#EXT-X-", 7))
      return true;
  return false;
}
streamoo_hls_result_t streamoo_hls_parse(char *doc, size_t bytes,
                                         const char *base,
                                         streamoo_hls_manifest_t *m) {
  if (!doc || !m || !bytes || bytes > STREAMOO_PLAYLIST_LIMIT ||
      memchr(doc, 0, bytes) || !streamoo_stream_url_allowed(base))
    return STREAMOO_HLS_INVALID;
  memset(m, 0, sizeof *m);
  doc[bytes] = 0;
  size_t at = bytes >= 3 && !memcmp(doc, "\xef\xbb\xbf", 3) ? 3 : 0;
  bool first = true, pending = false, variant = false, allow_variant = false,
       discontinuity = false, sequence_seen = false;
  uint64_t best_bandwidth = UINT64_MAX, bandwidth = 0;
  while (at < bytes) {
    char *line = doc + at;
    while (at < bytes && doc[at] != '\r' && doc[at] != '\n')
      ++at;
    if (at < bytes)
      doc[at++] = 0;
    if (!*line)
      continue;
    if (first) {
      first = false;
      if (strcmp(line, "#EXTM3U"))
        return STREAMOO_HLS_INVALID;
      continue;
    }
    if (!strncmp(line, "#EXT-X-KEY:", 11)) {
      char method[32];
      if (!attribute(line + 11, "METHOD", method, sizeof method) ||
          strcmp(method, "NONE"))
        return STREAMOO_HLS_UNSUPPORTED;
    } else if (!strncmp(line, "#EXT-X-MAP:", 11) ||
               !strncmp(line, "#EXT-X-BYTERANGE:", 17) ||
               !strncmp(line, "#EXT-X-SESSION-KEY:", 19) ||
               !strncmp(line, "#EXT-X-PART:", 12) ||
               !strcmp(line, "#EXT-X-I-FRAMES-ONLY") || !strcmp(line, "#EXT-X-GAP"))
      return STREAMOO_HLS_UNSUPPORTED;
    else if (!strncmp(line, "#EXT-X-TARGETDURATION:", 22)) {
      uint64_t n;
      if (!number(line + 22, &n) || !n || n > 120)
        return STREAMOO_HLS_INVALID;
      m->target_s = (uint32_t)n;
    } else if (!strncmp(line, "#EXT-X-MEDIA-SEQUENCE:", 22)) {
      if (sequence_seen || m->count || !number(line + 22, &m->sequence))
        return STREAMOO_HLS_INVALID;
      sequence_seen = true;
    } else if (!strcmp(line, "#EXT-X-ENDLIST"))
      m->endlist = true;
    else if (!strcmp(line, "#EXT-X-DISCONTINUITY"))
      discontinuity = true;
    else if (!strncmp(line, "#EXT-X-MEDIA:", 13)) {
      char type[32], uri[2049], def[8];
      if (attribute(line + 13, "TYPE", type, sizeof type) &&
          !strcmp(type, "AUDIO") &&
          attribute(line + 13, "URI", uri, sizeof uri)) {
        if (!m->variant[0] ||
            (attribute(line + 13, "DEFAULT", def, sizeof def) &&
             !strcmp(def, "YES"))) {
          if (!streamoo_url_resolve(base, uri, m->variant))
            return STREAMOO_HLS_INVALID;
          best_bandwidth = 0;
        }
      }
    } else if (!strncmp(line, "#EXT-X-STREAM-INF:", 18)) {
      char text[256], resolution[64];
      uint64_t n;
      variant = true;
      allow_variant = attribute(line + 18, "BANDWIDTH", text, sizeof text) &&
                      number(text, &n) && n > 0 && n <= 512000;
      bandwidth = allow_variant ? n : UINT64_MAX;
      if (attribute(line + 18, "RESOLUTION", resolution, sizeof resolution))
        allow_variant = false;
      if (attribute(line + 18, "CODECS", text, sizeof text) &&
          (!strstr(text, "mp4a") || strchr(text, ',')))
        allow_variant = false;
    } else if (!strncmp(line, "#EXTINF:", 8)) {
      const char *p = line + 8;
      bool digit = false, positive = false, dot = false;
      unsigned seconds = 0;
      for (; *p && *p != ','; ++p) {
        if (*p == '.' && !dot) {
          dot = true;
          continue;
        }
        if (*p < '0' || *p > '9')
          return STREAMOO_HLS_INVALID;
        digit = true;
        if (*p != '0')
          positive = true;
        if (!dot) {
          seconds = seconds * 10 + (unsigned)(*p - '0');
          if (seconds > 120)
            return STREAMOO_HLS_UNSUPPORTED;
        }
      }
      if (!digit || !positive || *p != ',')
        return STREAMOO_HLS_INVALID;
      pending = true;
    } else if (*line != '#') {
      if (variant) {
        if (allow_variant && bandwidth < best_bandwidth) {
          if (!streamoo_url_resolve(base, line, m->variant))
            return STREAMOO_HLS_INVALID;
          best_bandwidth = bandwidth;
        }
        variant = false;
      } else {
        if (!pending || m->count >= STREAMOO_HLS_SEGMENTS ||
            m->sequence > UINT64_MAX - m->count)
          return STREAMOO_HLS_INVALID;
        streamoo_hls_segment_t *segment = &m->segments[m->count++];
        if (!streamoo_url_resolve(base, line, segment->url))
          return STREAMOO_HLS_INVALID;
        segment->discontinuity = discontinuity;
        discontinuity = false;
        pending = false;
      }
    }
  }
  if (pending || variant || first || (m->count && m->variant[0]))
    return STREAMOO_HLS_INVALID;
  if (m->variant[0])
    return STREAMOO_HLS_MASTER;
  return m->count && m->target_s ? STREAMOO_HLS_MEDIA : STREAMOO_HLS_INVALID;
}
