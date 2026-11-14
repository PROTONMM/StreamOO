#include "ui_list_probe.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"

static const streamoo_ui_station_t stations[] = {
    {1001, (const uint8_t *)"TEST: Pierwsza", sizeof("TEST: Pierwsza") - 1},
    {1002, (const uint8_t *)"TEST: Łódź", sizeof("TEST: Łódź") - 1},
    {1003, (const uint8_t *)"TEST: Gdańsk", sizeof("TEST: Gdańsk") - 1},
    {1004, (const uint8_t *)"TEST: Poznań", sizeof("TEST: Poznań") - 1},
    {1005, (const uint8_t *)"TEST: Żory", sizeof("TEST: Żory") - 1},
};

static bool record_selection(void *context, const streamoo_ui_select_command_t *command)
{
    streamoo_ui_list_probe_t *probe = context;
    probe->requested_id = command->station_id;
    return true; /* Diagnostic command sink, not a player queue. */
}

bool streamoo_ui_list_probe_init(streamoo_ui_list_probe_t *probe)
{
    if (!probe) return false;
    *probe = (streamoo_ui_list_probe_t){0};
    if (!streamoo_ui_init(&probe->ui, 160, record_selection, probe)) return false;
    if (!streamoo_ui_set_catalog(&probe->ui, stations, 5, 1)) return false;
    streamoo_ui_set_playing(&probe->ui, 1002); /* Synthetic snapshot, no audio. */
    return streamoo_ui_open_stations(&probe->ui, 0);
}

bool streamoo_ui_list_probe_draw(streamoo_ui_list_probe_t *probe,
                                  uint8_t *frame, size_t bytes, uint64_t elapsed)
{
    if (!probe) return false;
    const struct { uint16_t at; int32_t delta; } moves[] = {
        {1000, 1}, {2000, 1}, {3000, 100}, {4000, -2},
    };
    while (probe->next_event < 4 && elapsed >= moves[probe->next_event].at) {
        if (!streamoo_ui_move(&probe->ui, moves[probe->next_event].delta,
                               moves[probe->next_event].at)) return false;
        ++probe->next_event;
    }
    if (probe->next_event == 4 && elapsed >= 5500) {
        if (!streamoo_ui_confirm(&probe->ui)) return false;
        ++probe->next_event;
    }
    if (probe->next_event == 5 && elapsed >= 6500) {
        if (!streamoo_ui_open_stations(&probe->ui, 6500)) return false;
        ++probe->next_event;
    }
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas, frame, bytes, 256, 64, 128)) return false;
    streamoo_canvas_fill(&canvas, (streamoo_rect_t){0, 0, 256, 64}, 0);
    const uint8_t label[] = "TEST - lista stacji";
    if (!streamoo_text_line(&canvas, (streamoo_rect_t){0, 0, 256, 8}, label, sizeof(label) - 1,
                             STREAMOO_TEXT_CENTER, 0, 0, 0, 10, streamoo_font5x8, NULL)) return false;
    if (probe->ui.screen == STREAMOO_UI_STATIONS)
        return streamoo_ui_draw_list(&probe->ui, &canvas, (streamoo_rect_t){0, 8, 256, 56}, elapsed);
    const uint8_t message[] = "TEST: wybór wysłany, bez audio";
    return streamoo_text_line(&canvas, (streamoo_rect_t){0, 8, 256, 56}, message,
                               sizeof(message) - 1, STREAMOO_TEXT_CENTER, 0, 0, 0,
                               15, streamoo_font5x8, NULL);
}
