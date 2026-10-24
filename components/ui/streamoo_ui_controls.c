#include "streamoo_ui_controls.h"

bool streamoo_ui_handle_input(streamoo_ui_t *ui, streamoo_input_event_t e,
                               streamoo_control_mode_t mode, streamoo_control_post_fn post,
                               void *context, uint64_t now)
{
    if (!ui || !ui->post || !post || mode < STREAMOO_CONTROL_MODE_A ||
        mode > STREAMOO_CONTROL_MODE_B || e.kind < STREAMOO_INPUT_NONE ||
        e.kind > STREAMOO_INPUT_PRESSED_ROTATE) return false;
    if (e.kind == STREAMOO_INPUT_NONE) return true;
    if (ui->screen == STREAMOO_UI_STATIONS) {
        switch (e.kind) {
        case STREAMOO_INPUT_ROTATE: return !ui->count || streamoo_ui_move(ui, e.delta, now);
        case STREAMOO_INPUT_SHORT: return streamoo_ui_confirm(ui);
        case STREAMOO_INPUT_LONG: return streamoo_ui_cancel(ui);
        default: return true;
        }
    }
    if (ui->screen != STREAMOO_UI_HOME) return false;
    if (e.kind == STREAMOO_INPUT_PRESSED_ROTATE) {
        int next = (int)(((int64_t)ui->visualization + e.delta) % 3);
        if (next < 0) next += 3;
        return streamoo_ui_set_visualization(ui, next);
    }
    streamoo_control_command_t cmd = {0};
    if (e.kind == STREAMOO_INPUT_ROTATE) {
        if (!e.delta) return true;
        cmd.kind = STREAMOO_CONTROL_VOLUME_DELTA;
        cmd.delta = e.delta;
    } else if ((e.kind == STREAMOO_INPUT_SHORT && mode == STREAMOO_CONTROL_MODE_A) ||
               (e.kind == STREAMOO_INPUT_LONG && mode == STREAMOO_CONTROL_MODE_B)) {
        return streamoo_ui_open_stations(ui, now);
    } else {
        cmd.kind = STREAMOO_CONTROL_TOGGLE;
    }
    return post(context, &cmd);
}
