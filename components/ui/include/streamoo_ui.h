#pragma once
#include "streamoo_canvas.h"
#include "streamoo_home_model.h"

/* Borrowed immutable catalog snapshot. IDs must be nonzero and unique;
 * entries/names stay alive until the next catalog replacement. The station
 * repository, not this UI, validates uniqueness and owns the data. */
typedef struct {
    uint64_t stable_id;
    const uint8_t *name;
    size_t name_bytes;
} streamoo_ui_station_t;

typedef enum { STREAMOO_UI_HOME, STREAMOO_UI_STATIONS } streamoo_ui_screen_t;
typedef struct {
    uint64_t station_id;
    uint64_t catalog_generation;
} streamoo_ui_select_command_t;
/* Nonblocking, non-reentrant queue adapter. Copies the command before returning.
 * false means queue full/unavailable; UI keeps the list open for retry. */
typedef bool (*streamoo_ui_post_fn)(void *context, const streamoo_ui_select_command_t *command);

typedef struct {
    streamoo_ui_screen_t screen;
    const streamoo_ui_station_t *stations;
    uint32_t count, selected;
    uint64_t catalog_generation, playing_id;
    int64_t animation_from_q16;
    uint64_t animation_start_ms, selection_start_ms;
    uint16_t animation_ms;
    streamoo_ui_post_fn post;
    void *post_context;
    streamoo_home_snapshot_t home;
    uint64_t home_text_start_ms;
    bool home_received, home_scroll_reset_pending;
    streamoo_visualization_kind_t visualization;
    bool running;
    uint64_t last_activity_ms;
} streamoo_ui_t;

/* Single UI-task ownership; no GPIO/player/network/RTOS dependency. All times
 * monotonic milliseconds. No allocation or I/O. animation_ms=0 snaps instantly. */
bool streamoo_ui_init(streamoo_ui_t *ui, uint16_t animation_ms,
                       streamoo_ui_post_fn post, void *context);
/* Replacing a snapshot closes browsing without issuing a player command.
 * After success the old snapshot may be released by its owner. */
bool streamoo_ui_set_catalog(streamoo_ui_t *ui, const streamoo_ui_station_t *stations,
                              uint32_t count, uint64_t generation);
void streamoo_ui_set_playing(streamoo_ui_t *ui, uint64_t station_id);
bool streamoo_ui_open_stations(streamoo_ui_t *ui, uint64_t now_ms);
bool streamoo_ui_move(streamoo_ui_t *ui, int32_t detents, uint64_t now_ms);
bool streamoo_ui_cancel(streamoo_ui_t *ui);
bool streamoo_ui_confirm(streamoo_ui_t *ui);
/* Logical target and animated position are independent. Confirm selects the
 * latest target even if motion is in progress; it never changes playing_id. */
int64_t streamoo_ui_position_q16(const streamoo_ui_t *ui, uint64_t now_ms);
/* List bounds inside canvas, min 8x16. Clipping, central selection, dimmed
 * neighbours; empty catalog has a message. Only draws STATIONS screen. */
bool streamoo_ui_draw_list(const streamoo_ui_t *ui, streamoo_canvas_t *canvas,
                            streamoo_rect_t bounds, uint64_t now_ms);

/* Apply a coherent snapshot in the UI task. Obsolete stream data are cleared
 * before storing, not left as borrowed stale pointers. Does not issue commands.
 * Invalid active fields reject the whole update without changing state. */
bool streamoo_ui_update_home(streamoo_ui_t *ui, const streamoo_home_snapshot_t *snapshot,
                              uint64_t now_ms);
bool streamoo_ui_set_visualization(streamoo_ui_t *ui, streamoo_visualization_kind_t kind);
/* Fixed 256x64 HOME geometry, respecting inherited clipping. No device I/O,
 * clock/network access or allocation. Unknown information has explicit placeholders. */
bool streamoo_ui_draw_home(const streamoo_ui_t *ui, streamoo_canvas_t *canvas, uint64_t now_ms);
