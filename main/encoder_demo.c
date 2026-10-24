#include "encoder_demo.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"
#include <stdio.h>

static const streamoo_ui_station_t stations[] = {
    {1001, (const uint8_t *)"TEST: Pierwsza", sizeof("TEST: Pierwsza")-1},
    {1002, (const uint8_t *)"TEST: Łódź", sizeof("TEST: Łódź")-1},
    {1003, (const uint8_t *)"TEST: Gdańsk", sizeof("TEST: Gdańsk")-1},
    {1004, (const uint8_t *)"TEST: Poznań", sizeof("TEST: Poznań")-1},
    {1005, (const uint8_t *)"TEST: Żory", sizeof("TEST: Żory")-1},
};
static bool select_station(void *context, const streamoo_ui_select_command_t *cmd)
{
    streamoo_encoder_demo_t *d = context;
    if (d->selection_pending) return false;
    d->selection = *cmd;
    d->selection_pending = true;
    return true;
}
static bool post_control(void *context, const streamoo_control_command_t *cmd)
{
    streamoo_encoder_demo_t *d = context;
    if (d->control_pending) return false;
    d->control = *cmd;
    d->control_pending = true;
    return true;
}
bool streamoo_encoder_demo_init(streamoo_encoder_demo_t *d)
{
    if (!d) return false;
    *d = (streamoo_encoder_demo_t){.station_id=1002, .generation=1, .volume=50, .running=true};
    if (!streamoo_ui_init(&d->ui, 160, select_station, d) ||
        !streamoo_ui_set_catalog(&d->ui, stations, 5, 1)) return false;
    streamoo_ui_set_playing(&d->ui, d->station_id);
    return true;
}
bool streamoo_encoder_demo_input(streamoo_encoder_demo_t *d, streamoo_input_event_t e,
                                  streamoo_control_mode_t mode, uint64_t now)
{
    if (!d || !streamoo_ui_handle_input(&d->ui, e, mode, post_control, d, now)) return false;
    /* Diagnostic owner applies copied commands after the UI callback returns.
     * Production adapter will enqueue to player_manager instead. */
    if (d->selection_pending) {
        if (d->selection.catalog_generation == d->ui.catalog_generation) {
            d->station_id = d->selection.station_id;
            ++d->generation;
        }
        d->selection_pending = false;
    }
    if (d->control_pending) {
        if (d->control.kind == STREAMOO_CONTROL_TOGGLE) d->running = !d->running;
        else {
            int64_t volume = (int64_t)d->volume + d->control.delta;
            d->volume = volume < 0 ? 0 : volume > 100 ? 100 : (int)volume;
        }
        d->control_pending = false;
    }
    /* Sync TEST source before next gesture, including open-list selection. */
    streamoo_ui_set_playing(&d->ui, d->station_id);
    return true;
}
bool streamoo_encoder_demo_draw(streamoo_encoder_demo_t *d, uint8_t *frame,
                                 size_t bytes, uint64_t now)
{
    if (!d) return false;
    streamoo_home_snapshot_t s = {
        .station_id=d->station_id, .stream_generation=d->generation,
        .data_station_id=d->station_id, .data_generation=d->generation,
        .text_revision=d->generation, .metadata_valid=true,
        .visualization_valid=true, .visualization={.band_count=24},
    };
    for (unsigned i=0; i<5; ++i)
        if (stations[i].stable_id == d->station_id)
            s.station_name=(streamoo_ui_text_t){stations[i].name,stations[i].name_bytes};
    const char *title="TEST sterowania - bez audio";
    s.title=(streamoo_ui_text_t){(const uint8_t *)title, sizeof("TEST sterowania - bez audio")-1};
    for (unsigned i=0; i<24; ++i) {
        unsigned phase=(unsigned)(now%2000+i*61)%2000;
        s.visualization.bands[i]=d->running ? (phase<1000 ? phase : 2000-phase)*32 : 0;
    }
    s.visualization.rms[0]=s.visualization.bands[4];
    s.visualization.rms[1]=s.visualization.bands[12];
    s.visualization.peak[0]=s.visualization.rms[0];
    s.visualization.peak[1]=s.visualization.rms[1];
    if (!streamoo_ui_update_home(&d->ui,&s,now)) return false;
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas,frame,bytes,256,64,128)) return false;
    if (d->ui.screen == STREAMOO_UI_STATIONS) {
        streamoo_canvas_fill(&canvas,(streamoo_rect_t){0,0,256,64},0);
        const uint8_t label[]="TEST - lista stacji";
        return streamoo_text_line(&canvas,(streamoo_rect_t){0,0,256,8},label,sizeof(label)-1,
            STREAMOO_TEXT_CENTER,0,0,0,10,streamoo_font5x8,NULL) &&
            streamoo_ui_draw_list(&d->ui,&canvas,(streamoo_rect_t){0,8,256,56},now);
    }
    if (!streamoo_ui_draw_home(&d->ui,&canvas,now)) return false;
    char status[44];
    int length=snprintf(status,sizeof(status),"TEST VOL %d  %s  W%u",d->volume,
                        d->running ? "PLAY" : "STOP",(unsigned)d->ui.visualization+1);
    if (length<0 || (size_t)length>=sizeof(status)) return false;
    /* Explicit test status in description row, outside production renderer. */
    streamoo_canvas_fill(&canvas,(streamoo_rect_t){0,24,256,8},0);
    return streamoo_text_line(&canvas,(streamoo_rect_t){0,24,256,8},
        (const uint8_t *)status,(size_t)length,STREAMOO_TEXT_CENTER,0,0,0,12,streamoo_font5x8,NULL);
}
