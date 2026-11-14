#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STREAMOO_LEVELS_BLOCK_MAX 240u
#define STREAMOO_LEVELS_WINDOW_MAX 48000u
#define STREAMOO_LEVELS_WINDOW_DEFAULT 1920u /* 40ms at the output's 48kHz. */

typedef struct {
    uint16_t rms[2], peak[2]; /* Linear amplitude Q15, saturated to 32767. */
    bool valid;
    uint64_t windows;
    uint32_t discontinuities;
} streamoo_levels_result_t;
typedef struct {
    uint64_t squares[2];
    uint32_t peak[2], frames, window_frames, previous_sequence;
    bool have_sequence;
    streamoo_levels_result_t result;
} streamoo_levels_t;

/* Single analysis-task owner, not an ISR API. No allocation, I/O, RTOS or gain.
 * Input: interleaved signed32 stereo BEFORE volume, fixed 48kHz; input is never
 * changed. Q15 conversion divides by65536 toward zero (exact for signed16<<16).
 * Windows are non-overlapping, 1..48000 frames; default1920 =40ms.
 * One call is one sequenced block, 1..240 frames. A gap/duplicate resets the
 * partial window and invalidates the old result before accepting this block.
 * uint32 sequence rollover is continuous. Latest COMPLETE window persists
 * between completions; windows counts completions, not calls. Stop/station
 * changes require init by the owner. Invalid calls do not modify the context.
 * This component does not provide smoothing, peak hold, dB/VU or FFT. */
bool streamoo_levels_init(streamoo_levels_t *, uint32_t window_frames);
bool streamoo_levels_process(streamoo_levels_t *, const int32_t *lr,
                             size_t frames, uint32_t sequence);
