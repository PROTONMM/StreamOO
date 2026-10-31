#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct {
  uint32_t interval, audio_left, metadata_left, used, revision;
  uint8_t metadata[4081];
  char title[257];
  uint64_t audio_total, title_audio_offset;
} streamoo_icy_t;
bool streamoo_icy_init(streamoo_icy_t *, uint32_t interval);
/* Whole arbitrary fragment; output capacity >= input bytes, strips ICY blocks.
 * No allocation/I/O. NUL/control bytes excluded from published title. */
bool streamoo_icy_feed(streamoo_icy_t *, const uint8_t *, size_t, uint8_t *,
                       size_t, size_t *);
