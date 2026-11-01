#include "streamoo_ui.h"

bool streamoo_ui_init(streamoo_ui_t *ui, uint16_t animation_ms,
                       streamoo_ui_post_fn post, void *context)
{
    if (!ui) return false;
    *ui = (streamoo_ui_t){0};
    if (!post) return false;
    ui->animation_ms = animation_ms;
    ui->running = true;
    ui->post = post;
    ui->post_context = context;
    return true;
}

bool streamoo_ui_set_catalog(streamoo_ui_t *ui, const streamoo_ui_station_t *stations,
                              uint32_t count, uint64_t generation)
{
    if (!ui || !ui->post || (!stations && count)) return false;
    for (uint32_t i = 0; i < count; ++i) {
        if (!stations[i].stable_id || (!stations[i].name && stations[i].name_bytes)) return false;
    }
    ui->stations = stations;
    ui->count = count;
    ui->catalog_generation = generation;
    ui->screen = STREAMOO_UI_HOME;
    ui->selected = 0;
    ui->animation_from_q16 = 0;
    return true;
}

void streamoo_ui_set_playing(streamoo_ui_t *ui, uint64_t station_id)
{
    if (!ui || !ui->post || ui->playing_id == station_id) return;
    ui->playing_id = station_id;
    ui->home.station_id = station_id;
    ui->home.station_name = (streamoo_ui_text_t){0};
    ui->home.title = ui->home.description = ui->home.codec = (streamoo_ui_text_t){0};
    ui->home.metadata_valid = ui->home.audio_info_valid = ui->home.visualization_valid = false;
    ui->home.bitrate_kbps = 0;
    ui->home.bitrate_measured = false;
    ui->home.visualization = (streamoo_visualization_model_t){0};
    ui->visualization_valid_at_ms = 0;
    ui->visualization_update_at_ms = 0;
    ui->home_scroll_reset_pending = true;
}

bool streamoo_ui_open_stations(streamoo_ui_t *ui, uint64_t now)
{
    if (!ui || !ui->post) return false;
    if (ui->screen == STREAMOO_UI_STATIONS) return true;
    ui->selected = 0;
    for (uint32_t i = 0; i < ui->count; ++i) {
        if (ui->stations[i].stable_id == ui->playing_id) {
            ui->selected = i;
            break;
        }
    }
    ui->animation_from_q16 = (int64_t)ui->selected * 65536;
    ui->animation_start_ms = ui->selection_start_ms = now;
    ui->last_activity_ms=now;
    ui->screen = STREAMOO_UI_STATIONS;
    return true;
}

int64_t streamoo_ui_position_q16(const streamoo_ui_t *ui, uint64_t now)
{
    if (!ui || !ui->post) return 0;
    const int64_t target = (int64_t)ui->selected * 65536;
    if (!ui->animation_ms) return target;
    const uint64_t elapsed = now >= ui->animation_start_ms ? now - ui->animation_start_ms : 0;
    if (elapsed >= ui->animation_ms) return target;
    const int64_t delta = target - ui->animation_from_q16;
    /* Divide before multiplying so large catalog jumps cannot overflow. */
    const int64_t step = (delta / ui->animation_ms) * (int64_t)elapsed +
                        (delta % ui->animation_ms) * (int64_t)elapsed / ui->animation_ms;
    return ui->animation_from_q16 + step;
}

bool streamoo_ui_move(streamoo_ui_t *ui, int32_t detents, uint64_t now)
{
    if (!ui || !ui->post || ui->screen != STREAMOO_UI_STATIONS || !ui->count) return false;
    int64_t target = (int64_t)ui->selected + detents;
    if (target < 0) target = 0;
    if (target >= ui->count) target = ui->count - 1;
    if ((uint32_t)target == ui->selected) return true;
    ui->animation_from_q16 = streamoo_ui_position_q16(ui, now);
    ui->selected = target;
    ui->animation_start_ms = ui->selection_start_ms = now;
    return true;
}

bool streamoo_ui_cancel(streamoo_ui_t *ui)
{
    if (!ui || !ui->post || ui->screen != STREAMOO_UI_STATIONS) return false;
    ui->screen = STREAMOO_UI_HOME;
    return true;
}

bool streamoo_ui_confirm(streamoo_ui_t *ui)
{
    if (!ui || !ui->post || ui->screen != STREAMOO_UI_STATIONS || !ui->count) return false;
    const streamoo_ui_select_command_t command = {
        ui->stations[ui->selected].stable_id, ui->catalog_generation,
    };
    if (!(ui->running && command.station_id==ui->playing_id) &&
        !ui->post(ui->post_context, &command)) return false;
    ui->screen = STREAMOO_UI_HOME;
    return true;
}

bool streamoo_ui_set_visualization(streamoo_ui_t *ui, streamoo_visualization_kind_t kind)
{
    if (!ui || !ui->post || kind < STREAMOO_VIS_SPECTRUM || kind > STREAMOO_VIS_ANALOG) return false;
    ui->visualization = kind;
    return true;
}
