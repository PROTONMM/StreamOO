#include "streamoo_catalog_format.h"
#include <string.h>
bool streamoo_catalog_decode_ids(uint8_t *data, size_t *bytes, uint64_t *ids,
                                 size_t capacity, size_t *rows,
                                 bool allow_new) {
  if (!data || !bytes || !ids || !rows)
    return false;
  size_t start = 0, write = 0;
  *rows = 0;
  while (start < *bytes) {
    size_t end = start;
    while (end < *bytes && data[end] != '\n')
      ++end;
    if (end - start < 18 || *rows == capacity || data[start + 16] != '\t')
      return false;
    uint64_t id = 0;
    for (unsigned i = 0; i < 16; ++i) {
      unsigned c = data[start + i], digit;
      if (c >= '0' && c <= '9')
        digit = c - '0';
      else if (c >= 'a' && c <= 'f')
        digit = c - 'a' + 10;
      else
        return false;
      id = (id << 4) | digit;
    }
    if (!id && !allow_new)
      return false;
    ids[(*rows)++] = id;
    size_t n = end - start - 17;
    memmove(data + write, data + start + 17, n);
    write += n;
    if (end < *bytes)
      data[write++] = '\n';
    start = end + 1;
  }
  *bytes = write;
  return true;
}
