#include "streamoo_config_model.h"
#include "cJSON.h"
#include <string.h>
static bool utf8(const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n;) {
    uint32_t c = p[i++];
    unsigned extra = 0;
    if (c < 0x80) {
      if (c < 32 || c == 127)
        return false;
      continue;
    }
    uint32_t minimum;
    if (c >= 0xc2 && c <= 0xdf) {
      extra = 1;
      minimum = 0x80;
      c &= 31;
    } else if (c >= 0xe0 && c <= 0xef) {
      extra = 2;
      minimum = 0x800;
      c &= 15;
    } else if (c >= 0xf0 && c <= 0xf4) {
      extra = 3;
      minimum = 0x10000;
      c &= 7;
    } else
      return false;
    if (extra > n - i)
      return false;
    while (extra--) {
      uint8_t b = p[i++];
      if ((b & 0xc0) != 0x80)
        return false;
      c = (c << 6) | (b & 63);
    }
    if (c < minimum || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff) ||
        c == 0x200b || c == 0xfeff)
      return false;
  }
  return true;
}
bool streamoo_wifi_profiles_valid(const streamoo_wifi_profiles_t *set) {
  if (!set || set->count > STREAMOO_WIFI_MAX)
    return false;
  for (unsigned i = 0; i < set->count; ++i) {
    size_t a = strnlen(set->profiles[i].ssid, 33),
           b = strnlen(set->profiles[i].password, 64);
    if (!a || a > 32 || b > 63 || (b && b < 8) ||
        !utf8((const uint8_t *)set->profiles[i].ssid, a) ||
        !utf8((const uint8_t *)set->profiles[i].password, b))
      return false;
    for (unsigned j = 0; j < i; ++j)
      if (!strcmp(set->profiles[i].ssid, set->profiles[j].ssid))
        return false;
  }
  return true;
}
bool streamoo_wifi_profiles_parse(const char *json, size_t bytes,
                                  const streamoo_wifi_profiles_t *old,
                                  streamoo_wifi_profiles_t *out) {
  if (!json || !out || !bytes || bytes > 4096 || memchr(json, 0, bytes))
    return false;
  for (size_t i = 0; i + 6 <= bytes; ++i)
    if (!memcmp(json + i, "\\u0000", 6))
      return false;
  if (old && !streamoo_wifi_profiles_valid(old))
    return false;
  unsigned depth = 0;
  bool quoted = false, escaped = false;
  for (size_t i = 0; i < bytes; ++i) {
    char c = json[i];
    if (quoted) {
      if (escaped)
        escaped = false;
      else if (c == '\\')
        escaped = true;
      else if (c == '"')
        quoted = false;
    } else if (c == '"')
      quoted = true;
    else if (c == '{' || c == '[') {
      if (++depth > 4)
        return false;
    } else if (c == '}' || c == ']') {
      if (!depth)
        return false;
      --depth;
    }
  }
  if (depth || quoted)
    return false;
  const char *end = NULL;
  cJSON *root = cJSON_ParseWithLengthOpts(json, bytes, &end, false);
  if (!root)
    return false;
  while (end < json + bytes &&
         (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t'))
    ++end;
  cJSON *array = cJSON_GetObjectItemCaseSensitive(root, "profiles");
  streamoo_wifi_profiles_t result = {0};
  bool ok = end == json + bytes && cJSON_IsObject(root) &&
            cJSON_IsArray(array) && cJSON_GetArraySize(array) <= 8;
  for (cJSON *field = root->child; field; field = field->next)
    if (!field->string || strcmp(field->string, "profiles") || field->next)
      ok = false;
  cJSON *item;
  cJSON_ArrayForEach(item, array) {
    if (!ok)
      break;
    unsigned ssid_fields = 0, password_fields = 0;
    for (cJSON *field = item->child; field; field = field->next) {
      if (field->string && !strcmp(field->string, "ssid"))
        ++ssid_fields;
      else if (field->string && !strcmp(field->string, "password"))
        ++password_fields;
      else
        ok = false;
    }
    if (ssid_fields != 1 || password_fields > 1)
      ok = false;
    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(item, "ssid"),
          *password = cJSON_GetObjectItemCaseSensitive(item, "password");
    if (!cJSON_IsObject(item) || !cJSON_IsString(ssid) ||
        strlen(ssid->valuestring) > 32 ||
        (password &&
         (!cJSON_IsString(password) || strlen(password->valuestring) > 63))) {
      ok = false;
      break;
    }
    streamoo_wifi_profile_t *p = &result.profiles[result.count++];
    strcpy(p->ssid, ssid->valuestring);
    bool found = password != NULL;
    if (password)
      strcpy(p->password, password->valuestring);
    else if (old)
      for (unsigned i = 0; i < old->count; ++i)
        if (!strcmp(old->profiles[i].ssid, p->ssid)) {
          strcpy(p->password, old->profiles[i].password);
          found = true;
          break;
        }
    if (!found) {
      ok = false;
      break;
    }
  }
  ok = ok && streamoo_wifi_profiles_valid(&result);
  cJSON_Delete(root);
  if (ok)
    *out = result;
  return ok;
}
static uint32_t checksum(const uint8_t *data, size_t n) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; ++i)
    h = (h ^ data[i]) * 16777619u;
  return h;
}
size_t streamoo_wifi_profiles_encode(const streamoo_wifi_profiles_t *s,
                                     uint8_t *out, size_t capacity) {
  if (!out || !streamoo_wifi_profiles_valid(s))
    return 0;
  size_t needed = 9;
  for (unsigned i = 0; i < s->count; ++i)
    needed += 2 + strlen(s->profiles[i].ssid) + strlen(s->profiles[i].password);
  if (capacity < needed)
    return 0;
  memcpy(out, "NWF1", 4);
  out[4] = s->count;
  size_t k = 5;
  for (unsigned i = 0; i < s->count; ++i) {
    size_t a = strlen(s->profiles[i].ssid), b = strlen(s->profiles[i].password);
    out[k++] = a;
    out[k++] = b;
    memcpy(out + k, s->profiles[i].ssid, a);
    k += a;
    memcpy(out + k, s->profiles[i].password, b);
    k += b;
  }
  uint32_t h = checksum(out, k);
  for (unsigned i = 0; i < 4; ++i)
    out[k++] = (uint8_t)(h >> (i * 8));
  return k;
}
bool streamoo_wifi_profiles_decode(const uint8_t *data, size_t n,
                                   streamoo_wifi_profiles_t *out) {
  if (!data || !out || n < 9 || memcmp(data, "NWF1", 4) || data[4] > 8)
    return false;
  uint32_t h = 0;
  for (unsigned i = 0; i < 4; ++i)
    h |= (uint32_t)data[n - 4 + i] << (i * 8);
  if (checksum(data, n - 4) != h)
    return false;
  streamoo_wifi_profiles_t s = {.count = data[4]};
  size_t k = 5;
  for (unsigned i = 0; i < s.count; ++i) {
    if (k + 2 > n - 4)
      return false;
    size_t a = data[k++], b = data[k++];
    if (!a || a > 32 || b > 63 || a + b > n - 4 - k ||
        memchr(data + k, 0, a + b))
      return false;
    memcpy(s.profiles[i].ssid, data + k, a);
    k += a;
    memcpy(s.profiles[i].password, data + k, b);
    k += b;
  }
  if (k != n - 4 || !streamoo_wifi_profiles_valid(&s))
    return false;
  *out = s;
  return true;
}
static bool integer(const uint8_t *p, size_t n) {
  if (!n || n > 11)
    return false;
  bool negative = p[0] == '-';
  size_t i = (p[0] == '-' || p[0] == '+') ? 1 : 0;
  if (i == n)
    return false;
  uint64_t v = 0;
  for (; i < n; ++i) {
    if (p[i] < '0' || p[i] > '9')
      return false;
    v = v * 10 + p[i] - '0';
    if (v > (negative ? 2147483648ULL : 2147483647ULL))
      return false;
  }
  return true;
}
static bool url_valid(const uint8_t *p, size_t n) {
  size_t scheme = n > 7 && !memcmp(p, "http://", 7)    ? 7
                  : n > 8 && !memcmp(p, "https://", 8) ? 8
                                                       : 0;
  if (!scheme)
    return false;
  for (size_t i = 0; i < n; ++i)
    if (p[i] <= 32 || p[i] >= 127 || p[i] == '@' || p[i] == '#')
      return false;
  size_t end = scheme;
  while (end < n && p[end] != '/' && p[end] != '?')
    ++end;
  if (end == scheme)
    return false;
  size_t colon = end;
  if (p[scheme] == '[') {
    size_t close = scheme + 1;
    while (close < end && p[close] != ']')
      ++close;
    if (close == end || close == scheme + 1)
      return false;
    if (close + 1 < end) {
      if (p[close + 1] != ':')
        return false;
      colon = close + 1;
    }
  } else {
    for (size_t i = scheme; i < end; ++i)
      if (p[i] == ':') {
        if (colon != end)
          return false;
        colon = i;
      }
  }
  if (colon == scheme)
    return false;
  if (colon < end) {
    if (colon + 1 == end)
      return false;
    uint32_t port = 0;
    for (size_t i = colon + 1; i < end; ++i) {
      if (p[i] < '0' || p[i] > '9')
        return false;
      port = port * 10 + p[i] - '0';
      if (port > 65535)
        return false;
    }
    if (!port)
      return false;
  }
  return true;
}
bool streamoo_playlist_validate(uint8_t *data, size_t *bytes,
                                streamoo_playlist_report_t *r,
                                streamoo_playlist_workspace_t *ws) {
  if (!data || !bytes || !r || !ws)
    return false;
  *r = (streamoo_playlist_report_t){0};
  size_t n = *bytes, read = 0, write = 0;
  if (!n || n > STREAMOO_PLAYLIST_MAX_BYTES) {
    r->error = "file_size";
    return false;
  }
  if (n >= 3 && !memcmp(data, "\xef\xbb\xbf", 3))
    read = 3;
  while (read < n) {
    size_t end = read;
    while (end < n && data[end] != '\n')
      ++end;
    size_t len = end - read;
    if (len && data[read + len - 1] == '\r')
      --len;
    ++r->rows;
    r->error_line = r->rows;
    if (r->rows > STREAMOO_PLAYLIST_MAX_ROWS) {
      r->error = "too_many_rows";
      return false;
    }
    size_t first = len, second = len;
    for (size_t i = 0; i < len; ++i)
      if (data[read + i] == '\t') {
        if (first == len)
          first = i;
        else if (second == len)
          second = i;
        else {
          r->error = "columns";
          return false;
        }
      }
    if (first == len || second == len || !first || first > 128) {
      r->error = "columns_or_name";
      return false;
    }
    if (!utf8(data + read, first)) {
      r->error = "name_utf8";
      return false;
    }
    size_t u = read + first + 1, ul = second - first - 1;
    bool normalized = false;
    while (ul >= 3 && !memcmp(data + u, "\xe2\x80\x8b", 3)) {
      u += 3;
      ul -= 3;
      normalized = true;
    }
    if (normalized)
      ++r->normalized;
    if (!ul || ul > 2048 || !url_valid(data + u, ul)) {
      r->error = "url";
      return false;
    }
    if (!integer(data + read + second + 1, len - second - 1)) {
      r->error = "ovol";
      return false;
    }
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < ul; ++i)
      h = (h ^ data[u + i]) * 1099511628211ULL;
    for (unsigned i = 0; i + 1 < r->rows; ++i)
      if (ws->seen[i].hash == h && ws->seen[i].bytes == ul &&
          !memcmp(data + ws->seen[i].offset, data + u, ul)) {
        ++r->duplicates;
        break;
      }
    memmove(data + write, data + read, first);
    write += first;
    data[write++] = '\t';
    ws->seen[r->rows - 1] = (streamoo_playlist_seen_t){h, write, ul};
    memmove(data + write, data + u, ul);
    write += ul;
    data[write++] = '\t';
    size_t vlen = len - second - 1;
    memmove(data + write, data + read + second + 1, vlen);
    write += vlen;
    if (end < n)
      data[write++] = '\n';
    read = end + 1;
  }
  r->error_line = 0;
  *bytes = write;
  if (!r->rows)
    r->error = "empty_file";
  return r->rows > 0;
}
