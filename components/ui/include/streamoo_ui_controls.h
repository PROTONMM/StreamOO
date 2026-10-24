#pragma once
#include "streamoo_ui.h"
#include "streamoo_input.h"

typedef enum { STREAMOO_CONTROL_MODE_A, STREAMOO_CONTROL_MODE_B } streamoo_control_mode_t;
typedef enum { STREAMOO_CONTROL_VOLUME_DELTA, STREAMOO_CONTROL_TOGGLE } streamoo_control_kind_t;
typedef struct { streamoo_control_kind_t kind; int delta; } streamoo_control_command_t;
/* Nonblocking copy into owner's command queue. false means not accepted.
 * UI never owns actual volume or playback state. */
typedef bool (*streamoo_control_post_fn)(void *, const streamoo_control_command_t *);
/* Single UI owner; HOME: volume/visualization and mode A/B clicks.
 * STATIONS: rotate browses, short confirms, long cancels; pressed-turn ignored.
 * false leaves state unchanged on rejected commands. No allocation or I/O. */
bool streamoo_ui_handle_input(streamoo_ui_t *ui, streamoo_input_event_t event,
                               streamoo_control_mode_t mode, streamoo_control_post_fn post,
                               void *context, uint64_t now_ms);
