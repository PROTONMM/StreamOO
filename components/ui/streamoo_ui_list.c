#include "streamoo_ui.h"
#include "streamoo_bitmap_font.h"
#include "streamoo_font5x8.h"

bool streamoo_ui_draw_list(const streamoo_ui_t *ui, streamoo_canvas_t *canvas,
                            streamoo_rect_t bounds, uint64_t now)
{
    if (!ui || !ui->post || !canvas || !canvas->pixels || ui->screen != STREAMOO_UI_STATIONS ||
        bounds.x < 0 || bounds.y < 0 || bounds.width < 8 || bounds.height < 16 ||
        (int32_t)bounds.x + bounds.width > canvas->width ||
        (int32_t)bounds.y + bounds.height > canvas->height) return false;
    streamoo_canvas_t child;
    if (!streamoo_canvas_child(canvas, bounds, &child)) return false;
    streamoo_canvas_fill(&child, bounds, 0);
    if (!ui->count) {
        const uint8_t message[] = "Brak stacji";
        return streamoo_text_line(&child, bounds, message, sizeof(message) - 1,
                                   STREAMOO_TEXT_CENTER, 0, 0, 0, 12, streamoo_font5x8, NULL);
    }
    const int64_t position = streamoo_ui_position_q16(ui, now);
    const int64_t center = position / 65536;
    const int64_t radius = bounds.height / 32 + 2;
    for (int64_t i = center - radius; i <= center + radius; ++i) {
        if (i < 0 || i >= ui->count) continue;
        const int64_t top = bounds.y + bounds.height / 2 - 8 + i * 16 - position * 16 / 65536;
        if (top + 16 <= child.clip.y || top >= (int32_t)child.clip.y + child.clip.height) continue;
        const streamoo_rect_t row = {bounds.x, top, bounds.width, 16};
        const streamoo_ui_station_t *station = &ui->stations[i];
        if (i == ui->selected) {
            const uint64_t elapsed = now >= ui->selection_start_ms ? now - ui->selection_start_ms : 0;
            if (!streamoo_bitmap_text_line(&child, row, streamoo_ui_font(),
                                           station->name, station->name_bytes, STREAMOO_TEXT_CENTER,
                                           elapsed, 30, 1000, 15)) return false;
        } else {
            const int64_t distance = i > center ? i - center : center - i;
            const uint8_t gray = distance <= 1 ? 8 : 4;
            if (!streamoo_text_line(&child, row, station->name, station->name_bytes,
                                    STREAMOO_TEXT_CENTER, 0, 0, 0, gray,
                                    streamoo_font5x8, NULL)) return false;
        }
    }
    return true;
}
