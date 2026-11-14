#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
  uint64_t stable_id;
  const uint8_t *name, *url;
  size_t name_bytes, url_bytes;
  int32_t ovol;
} streamoo_station_t;
/* Build bounded entries from a validated, canonical CSV COPY. Splits fields
 * in-place into NUL-terminated strings; output borrows this buffer.
 * IDs depend on name/URL and identical-row occurrence, never row position or
 * ovol. A detected 64-bit collision rejects the whole candidate. No alloc/I/O.
 */
bool streamoo_stations_build(uint8_t *data, size_t bytes,
                             streamoo_station_t *out, size_t capacity,
                             size_t *count);
