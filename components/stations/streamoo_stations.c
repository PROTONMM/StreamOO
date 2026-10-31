#include "streamoo_stations.h"
#include <string.h>
static uint64_t hash_bytes(uint64_t h, const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; ++i)
    h = (h ^ p[i]) * 1099511628211ULL;
  return h;
}
bool streamoo_stations_build(uint8_t *data, size_t bytes,
                             streamoo_station_t *out, size_t capacity,
                             size_t *count) {
  if (!data || !out || !count || !bytes)
    return false;
  *count = 0;
  size_t start = 0;
  while (start < bytes) {
    if (*count >= capacity)
      return false;
    size_t end = start, a = SIZE_MAX, b = SIZE_MAX;
    while (end < bytes && data[end] != '\n') {
      if (!data[end])
        return false;
      if (data[end] == '\t') {
        if (a == SIZE_MAX)
          a = end;
        else if (b == SIZE_MAX)
          b = end;
        else
          return false;
      }
      ++end;
    }
    if (a == SIZE_MAX || b == SIZE_MAX || a == start || b == a + 1 ||
        end == b + 1)
      return false;
    size_t i = b + 1;
    bool negative = data[i] == '-';
    if (data[i] == '-' || data[i] == '+')
      ++i;
    if (i == end)
      return false;
    uint64_t value = 0;
    for (; i < end; ++i) {
      if (data[i] < '0' || data[i] > '9')
        return false;
      value = value * 10 + data[i] - '0';
      if (value > (negative ? 2147483648ULL : 2147483647ULL))
        return false;
    }
    streamoo_station_t s = {
        .name = data + start,
        .name_bytes = a - start,
        .url = data + a + 1,
        .url_bytes = b - a - 1,
        .ovol = (int32_t)(negative ? -(int64_t)value : (int64_t)value)};
    uint32_t occurrence = 0;
    for (size_t j = 0; j < *count; ++j)
      if (out[j].name_bytes == s.name_bytes &&
          out[j].url_bytes == s.url_bytes &&
          !memcmp(out[j].name, s.name, s.name_bytes) &&
          !memcmp(out[j].url, s.url, s.url_bytes))
        ++occurrence;
    uint8_t ordinal[4];
    for (unsigned j = 0; j < 4; ++j)
      ordinal[j] = (uint8_t)(occurrence >> (8 * j));
    uint8_t zero = 0;
    uint64_t h = hash_bytes(14695981039346656037ULL, s.url, s.url_bytes);
    h = hash_bytes(h, &zero, 1);
    h = hash_bytes(h, s.name, s.name_bytes);
    h = hash_bytes(h, &zero, 1);
    s.stable_id = hash_bytes(h, ordinal, sizeof ordinal);
    if (!s.stable_id)
      return false;
    for (size_t j = 0; j < *count; ++j)
      if (out[j].stable_id == s.stable_id)
        return false;
    data[a] = data[b] = 0;
    if (end < bytes)
      data[end] = 0;
    out[(*count)++] = s;
    start = end + 1;
  }
  return true;
}
