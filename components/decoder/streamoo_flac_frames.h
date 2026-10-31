#pragma once
#include "streamoo_flac_header.h"
#include <stdbool.h>
typedef struct {
  uint8_t *data;
  size_t bytes, scan, ready;
  uint16_t crc;
} streamoo_flac_frames_t;
#define STREAMOO_FLAC_FRAME_STORAGE (STREAMOO_FLAC_MAX_FRAME_BYTES + 16u)
/* Original frame boundary scanner: CRC8-valid next header AND complete CRC16.
 * Storage allocated by decoder owner before output starts; no allocations here.
 */
void streamoo_flac_frames_init(streamoo_flac_frames_t *, uint8_t *storage);
size_t streamoo_flac_frames_append(streamoo_flac_frames_t *, const uint8_t *,
                                   size_t);
/* 0 needs input, 1 complete frame in data[0..ready), -1 malformed/limit. */
int streamoo_flac_frames_find(streamoo_flac_frames_t *, bool eos,
                              size_t pcm_capacity);
void streamoo_flac_frames_consume(streamoo_flac_frames_t *);
