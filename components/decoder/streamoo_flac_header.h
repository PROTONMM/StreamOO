#pragma once
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_FLAC_MAX_FRAME_BYTES (128u * 1024u)
/* Original RFC9639 header reader. No codec or frame-body implementation. */
typedef enum { STREAMOO_FLAC_MORE, STREAMOO_FLAC_FILE, STREAMOO_FLAC_FRAME, STREAMOO_FLAC_BAD } streamoo_flac_header_result_t;
/* At most16 input bytes. A complete CRC8-valid, self-describing frame header
 * produces42 bytes (fLaC + final STREAMINFO), without altering the frame. */
streamoo_flac_header_result_t streamoo_flac_header(const uint8_t *data,size_t bytes,
    size_t pcm_capacity,uint8_t streaminfo[42],size_t *header_bytes);
/* A later header may refer to STREAMINFO for rate/depth. CRC8 still required. */
streamoo_flac_header_result_t streamoo_flac_boundary_header(const uint8_t *data,
    size_t bytes,size_t *header_bytes);
