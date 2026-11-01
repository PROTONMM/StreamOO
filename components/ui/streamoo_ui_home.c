#include "streamoo_ui.h"
#include "streamoo_bitmap_font.h"
#include "streamoo_font5x8.h"

#define STREAMOO_UI_VISUALIZATION_HOLD_MS 800u

static bool text_valid(streamoo_ui_text_t text)
{
    return text.data || !text.bytes;
}

static bool data_match(const streamoo_home_snapshot_t *s)
{
    return s->station_id && s->data_station_id == s->station_id &&
           s->data_generation == s->stream_generation;
}

static bool clock_valid(streamoo_ui_clock_t c)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (c.year < 1 || c.year > 9999 || c.month < 1 || c.month > 12 ||
        c.hour > 23 || c.minute > 59 || c.second > 59) return false;
    unsigned max_day = days[c.month - 1];
    if (c.month == 2 && (c.year % 400 == 0 || (c.year % 4 == 0 && c.year % 100 != 0))) ++max_day;
    return c.day >= 1 && c.day <= max_day;
}

static bool levels_valid(const streamoo_visualization_model_t *model)
{
    if (model->band_count > STREAMOO_VIS_MAX_BANDS) return false;
    for (unsigned ch = 0; ch < 2; ++ch)
        if (model->rms[ch] > 32767 || model->peak[ch] > 32767) return false;
    for (unsigned i = 0; i < model->band_count; ++i)
        if (model->bands[i] > 32767) return false;
    return true;
}

static void decay_visualization(streamoo_visualization_model_t *model, uint64_t elapsed_ms)
{
    if (!model || elapsed_ms > STREAMOO_UI_VISUALIZATION_HOLD_MS) return;
    const uint32_t fall = (uint32_t)((elapsed_ms * 32767u +
                                      STREAMOO_UI_VISUALIZATION_HOLD_MS - 1u) /
                                     STREAMOO_UI_VISUALIZATION_HOLD_MS);
    for (unsigned channel = 0; channel < 2; ++channel) {
        model->rms[channel] = model->rms[channel] > fall ? model->rms[channel] - fall : 0;
        model->peak[channel] = model->peak[channel] > fall ? model->peak[channel] - fall : 0;
    }
    for (unsigned band = 0; band < model->band_count; ++band)
        model->bands[band] = model->bands[band] > fall ? model->bands[band] - fall : 0;
}

bool streamoo_ui_update_home(streamoo_ui_t *ui, const streamoo_home_snapshot_t *s, uint64_t now)
{
    if (!ui || !ui->post || !s || (s->station_id && !text_valid(s->station_name)) ||
        (s->wifi_connected && s->rssi_valid && (s->rssi_dbm < -127 || s->rssi_dbm > 0)) ||
        (s->clock_valid && !clock_valid(s->clock))) return false;
    if (ui->home_received) {
        if (s->stream_generation < ui->home.stream_generation ||
            (ui->home_scroll_reset_pending && s->stream_generation == ui->home.stream_generation)) return false;
        if (s->stream_generation == ui->home.stream_generation) {
            const uint64_t expected_id = ui->home_scroll_reset_pending ? ui->playing_id : ui->home.station_id;
            if (s->station_id != expected_id || s->text_revision < ui->home.text_revision) return false;
        }
    }
    const bool match = data_match(s);
    if (match && s->metadata_valid && (!text_valid(s->title) || !text_valid(s->description))) return false;
    if (match && s->audio_info_valid && !text_valid(s->codec)) return false;
    if (match && s->visualization_valid && !levels_valid(&s->visualization)) return false;
    const bool reset = !ui->home_received || ui->home_scroll_reset_pending ||
        ui->home.station_id != s->station_id || ui->home.stream_generation != s->stream_generation ||
        ui->home.text_revision != s->text_revision || data_match(&ui->home) != match ||
        ui->home.metadata_valid != (match && s->metadata_valid) ||
        ui->home.audio_info_valid != (match && s->audio_info_valid);
    const bool retain_visualization = ui->home_received && !ui->home_scroll_reset_pending && match &&
        ui->home.station_id == s->station_id &&
        ui->home.stream_generation == s->stream_generation && !s->visualization_valid &&
        ui->home.visualization_valid && ui->visualization_valid_kind == ui->visualization &&
        now >= ui->visualization_valid_at_ms &&
        now - ui->visualization_valid_at_ms <= STREAMOO_UI_VISUALIZATION_HOLD_MS;
    const streamoo_visualization_model_t previous_visualization = ui->home.visualization;
    ui->home = *s;
    if (!s->station_id) ui->home.station_name = (streamoo_ui_text_t){0};
    if (!match || !s->metadata_valid) {
        ui->home.metadata_valid = false;
        ui->home.title = ui->home.description = (streamoo_ui_text_t){0};
    }
    if (!match || !s->audio_info_valid) {
        ui->home.audio_info_valid = false;
        ui->home.codec = (streamoo_ui_text_t){0};
        ui->home.bitrate_kbps = 0;
        ui->home.bitrate_measured = false;
    }
    if (!match || !s->visualization_valid) {
        if (retain_visualization) {
            ui->home.visualization_valid = true;
            ui->home.visualization = previous_visualization;
            decay_visualization(&ui->home.visualization,
                                now - ui->visualization_update_at_ms);
            ui->visualization_update_at_ms = now;
        } else {
            ui->home.visualization_valid = false;
            ui->home.visualization = (streamoo_visualization_model_t){0};
        }
    } else {
        ui->visualization_valid_at_ms = now;
        ui->visualization_update_at_ms = now;
        ui->visualization_valid_kind = ui->visualization;
    }
    ui->playing_id = s->station_id;
    if (reset) ui->home_text_start_ms = now;
    ui->home_scroll_reset_pending = false;
    ui->home_received = true;
    return true;
}

static size_t decimal(uint8_t *out, uint32_t value)
{
    uint8_t reverse[10];
    size_t count = 0;
    do { reverse[count++] = '0' + value % 10; value /= 10; } while (value);
    for (size_t i = 0; i < count; ++i) out[i] = reverse[count - i - 1];
    return count;
}

static bool small(streamoo_canvas_t *c, streamoo_rect_t bounds, const uint8_t *text,
                  size_t bytes, streamoo_text_align_t align, uint64_t elapsed, uint8_t gray)
{
    return streamoo_text_line(c, bounds, text, bytes, align, elapsed, 30, 1000,
                              gray, streamoo_font5x8, NULL);
}

static bool info_panel(streamoo_canvas_t *c, const streamoo_home_snapshot_t *s)
{
    static const uint8_t unknown[] = "---";
    streamoo_ui_text_t codec = s->audio_info_valid && s->codec.bytes ? s->codec :
                               (streamoo_ui_text_t){unknown, 3};
    if (!small(c, (streamoo_rect_t){0, 32, 44, 8}, codec.data, codec.bytes,
                STREAMOO_TEXT_LEFT, 0, 12)) return false;
    uint8_t bitrate[12] = {'-', '-'};
    size_t bytes = 2;
    if (s->audio_info_valid && s->bitrate_kbps) {
        uint32_t value = s->bitrate_kbps;
        uint8_t unit = 'k';
        if (value >= 10000000) { value /= 1000000; unit = 'G'; }
        else if (value >= 10000) { value /= 1000; unit = 'M'; }
        size_t prefix=s->bitrate_measured ? 1 : 0;
        if(prefix)bitrate[0]='~';
        bytes = prefix + decimal(bitrate + prefix, value);
        bitrate[bytes++] = unit;
    }
    if (!small(c, (streamoo_rect_t){0, 40, 44, 8}, bitrate, bytes, STREAMOO_TEXT_LEFT, 0, 10)) return false;
    unsigned bars = 0;
    if (s->wifi_connected && s->rssi_valid)
        bars = 1 + (s->rssi_dbm >= -80) + (s->rssi_dbm >= -70) + (s->rssi_dbm >= -60);
    for (unsigned i = 0; i < 4; ++i) {
        const int16_t h = (i + 1) * 3;
        streamoo_canvas_fill(c, (streamoo_rect_t){1 + i * 3, 64 - h, 2, h}, i < bars ? 12 : 3);
    }
    uint8_t rssi[4] = {'O', 'F', 'F'};
    bytes = 3;
    if (!s->wifi_connected) {
        streamoo_canvas_line(c, 1, 51, 11, 63, 10);
        streamoo_canvas_line(c, 11, 51, 1, 63, 10);
    } else if (s->rssi_valid) {
        bytes = 0;
        if (s->rssi_dbm < 0) rssi[bytes++] = '-';
        bytes += decimal(rssi + bytes, s->rssi_dbm < 0 ? -s->rssi_dbm : s->rssi_dbm);
    } else { rssi[0] = '?'; bytes = 1; }
    return small(c, (streamoo_rect_t){14, 56, 24, 8}, rssi, bytes, STREAMOO_TEXT_RIGHT, 0, 10);
}

static void two_digits(uint8_t *out, unsigned value)
{
    out[0] = '0' + value / 10;
    out[1] = '0' + value % 10;
}

static bool clock_panel(streamoo_canvas_t *c, const streamoo_home_snapshot_t *s)
{
    uint8_t date[] = "--.--.----", time[] = "--:--:--";
    if (s->clock_valid) {
        two_digits(date, s->clock.day); two_digits(date + 3, s->clock.month);
        date[6] = '0' + s->clock.year / 1000;
        date[7] = '0' + s->clock.year / 100 % 10;
        date[8] = '0' + s->clock.year / 10 % 10;
        date[9] = '0' + s->clock.year % 10;
        two_digits(time, s->clock.hour); two_digits(time + 3, s->clock.minute);
        two_digits(time + 6, s->clock.second);
    }
    return small(c, (streamoo_rect_t){176, 32, 80, 8}, date, 10, STREAMOO_TEXT_CENTER, 0, 10) &&
        streamoo_bitmap_text_line(c, (streamoo_rect_t){176, 40, 80, 16}, streamoo_ui_font(),
                                  time, 8, STREAMOO_TEXT_CENTER, 0, 0, 0, 15);
}

bool streamoo_ui_draw_home(const streamoo_ui_t *ui, streamoo_canvas_t *canvas, uint64_t now)
{
    if (!ui || !ui->post || !canvas || !canvas->pixels || canvas->width < 256 ||
        canvas->height < 64 || ui->screen != STREAMOO_UI_HOME) return false;
    const streamoo_home_snapshot_t *s = &ui->home;
    streamoo_canvas_t c;
    if (!streamoo_canvas_child(canvas, (streamoo_rect_t){0, 0, 256, 64}, &c)) return false;
    streamoo_canvas_fill(&c, (streamoo_rect_t){0, 0, 256, 64}, 0);
    static const uint8_t no_station[] = "Brak stacji", no_name[] = "Brak nazwy";
    streamoo_ui_text_t name = s->station_name;
    if (!s->station_id) name = (streamoo_ui_text_t){no_station, sizeof(no_station) - 1};
    else if (!name.bytes) {
        name = (streamoo_ui_text_t){no_name, sizeof(no_name) - 1};
        for (uint32_t i = 0; i < ui->count; ++i) {
            if (ui->stations[i].stable_id == s->station_id) {
                name = (streamoo_ui_text_t){ui->stations[i].name, ui->stations[i].name_bytes};
                break;
            }
        }
    }
    const uint64_t elapsed = !ui->home_scroll_reset_pending && now >= ui->home_text_start_ms ?
                              now - ui->home_text_start_ms : 0;
    if (!streamoo_bitmap_text_line(&c, (streamoo_rect_t){0, 0, 256, 16}, streamoo_ui_font(),
                                    name.data, name.bytes, STREAMOO_TEXT_CENTER, elapsed,
                                    30, 1000, 15)) return false;
    if (!small(&c, (streamoo_rect_t){0, 16, 256, 8}, s->title.data, s->title.bytes,
                STREAMOO_TEXT_LEFT, elapsed, 12) ||
        !small(&c, (streamoo_rect_t){0, 24, 256, 8}, s->description.data, s->description.bytes,
                STREAMOO_TEXT_LEFT, elapsed, 10)) return false;
    if (!info_panel(&c, s) || !clock_panel(&c, s)) return false;
    if (s->visualization_valid)
        return streamoo_visualization_draw(&c, (streamoo_rect_t){44, 32, 132, 32}, ui->visualization,
                                            &s->visualization);
    static const uint8_t missing[] = "Brak danych";
    return small(&c, (streamoo_rect_t){44, 32, 132, 32}, missing, sizeof(missing) - 1,
                  STREAMOO_TEXT_CENTER, 0, 6);
}
