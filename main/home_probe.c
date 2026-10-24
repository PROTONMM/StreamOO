#include "home_probe.h"
#include "streamoo_text_layout.h"
#include "streamoo_font5x8.h"

#define TEXT(value) (streamoo_ui_text_t){(const uint8_t *)(value), sizeof(value) - 1}

static bool no_player(void *context, const streamoo_ui_select_command_t *command)
{
    (void)context;
    (void)command;
    return false;
}

bool streamoo_home_probe_init(streamoo_home_probe_t *probe)
{
    return probe && streamoo_ui_init(&probe->ui, 160, no_player, NULL);
}

static uint16_t triangle(uint32_t phase)
{
    phase %= 4000;
    return (phase < 2000 ? phase : 4000 - phase) * 32767 / 2000;
}

bool streamoo_home_probe_draw(streamoo_home_probe_t *probe, uint8_t *frame,
                               size_t bytes, uint64_t elapsed)
{
    if (!probe) return false;
    streamoo_home_snapshot_t s = {
        .station_id = 1002, .stream_generation = 1,
        .data_station_id = 1002, .data_generation = 1, .text_revision = 1,
        .station_name = TEXT("TEST: Radio Łódź"),
        .title = TEXT("TEST: Zażółć gęślą jaźń - StreamOO radio - test"),
        .description = TEXT("Dane testowe - bez odtwarzania audio"),
        .codec = TEXT("MP3"), .bitrate_kbps = 192,
        .wifi_connected = true, .rssi_valid = true, .rssi_dbm = -58,
        .metadata_valid = true, .audio_info_valid = true,
        .clock_valid = true, .clock = {2026, 10, 24, 12, 34, (elapsed / 1000) % 60},
        .visualization_valid = true, .visualization = {.band_count = 24},
    };
    const uint32_t phase = elapsed % 4000;
    s.visualization.rms[0] = triangle(phase);
    s.visualization.rms[1] = triangle(phase + 1000);
    for (unsigned channel = 0; channel < 2; ++channel) {
        const uint32_t peak = (uint32_t)s.visualization.rms[channel] + 8192;
        s.visualization.peak[channel] = peak > 32767 ? 32767 : peak;
    }
    for (unsigned band = 0; band < 24; ++band) s.visualization.bands[band] = triangle(phase + band * 110);
    if (elapsed >= 12000) {
        s.station_id = 1003;
        s.stream_generation = 2;
        s.text_revision = 2;
        s.station_name = TEXT("TEST: nowa stacja");
        /* Deliberately leave old stream tags/text to test stale-data removal. */
    }
    if (elapsed >= 15000) {
        s.data_station_id = s.station_id;
        s.data_generation = s.stream_generation;
        s.text_revision = 3;
        s.title = TEXT("TEST: metadane nowej stacji");
        s.description = TEXT("Nowy strumień - dane syntetyczne");
        s.codec = TEXT("FLAC");
        s.bitrate_kbps = 950;
    }
    if (elapsed >= 18000) {
        s.station_id = 0;
        s.stream_generation = 3;
        s.text_revision = 4;
        s.wifi_connected = s.rssi_valid = s.clock_valid = false;
    }
    if (!streamoo_ui_update_home(&probe->ui, &s, elapsed) ||
        !streamoo_ui_set_visualization(&probe->ui, (elapsed / 4000) % 3)) return false;
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128) ||
        !streamoo_ui_draw_home(&probe->ui, &canvas, elapsed)) return false;
    /* Explicit label only in diagnostic composition, not production renderer. */
    return streamoo_text_line(&canvas, (streamoo_rect_t){176, 56, 80, 8},
                               (const uint8_t *)"TEST", 4, STREAMOO_TEXT_CENTER,
                               0, 0, 0, 6, streamoo_font5x8, NULL);
}
