#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* FLAC network reserve: ~500ms at declared icy-br, otherwise64KiB.
 * Cap below the existing256KiB compressed ring; do not apply to other codecs.
 */
static inline size_t streamoo_player_reserve(bool flac, uint32_t kbps) {
  if (!flac)
    return 8192;
  uint64_t bytes = kbps ? (uint64_t)kbps * 125 / 2 : 65536;
  if (bytes < 32768)
    bytes = 32768;
  return bytes > 196608 ? 196608 : (size_t)bytes;
}
