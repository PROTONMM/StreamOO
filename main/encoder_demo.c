#include "encoder_demo.h"
#include "streamoo_font5x8.h"
#include "streamoo_text_layout.h"
#include "streamoo_ui_connection_row.h"
#include <stdio.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif
#if !defined(ESP_PLATFORM) || defined(CONFIG_STREAMOO_HARDWARE_TESTS)
#define SYNTHETIC_UI 1
#else
#define SYNTHETIC_UI 0
#endif

#if SYNTHETIC_UI
static const streamoo_ui_station_t stations[] = {
    {1001, (const uint8_t *)"TEST: Pierwsza", sizeof("TEST: Pierwsza")-1},
    {1002, (const uint8_t *)"TEST: Łódź", sizeof("TEST: Łódź")-1},
    {1003, (const uint8_t *)"TEST: Gdańsk", sizeof("TEST: Gdańsk")-1},
    {1004, (const uint8_t *)"TEST: Poznań", sizeof("TEST: Poznań")-1},
    {1005, (const uint8_t *)"TEST: Żory", sizeof("TEST: Żory")-1},
};
#endif
static bool select_station(void *context, const streamoo_ui_select_command_t *cmd)
{
    streamoo_encoder_demo_t *d = context;
    if(d->station_post) return d->station_post(d->station_context,cmd);
#if SYNTHETIC_UI
    if (d->selection_pending) return false;
    d->selection = *cmd;
    d->selection_pending = true;
    return true;
#else
    return false;
#endif
}
static bool post_control(void *context, const streamoo_control_command_t *cmd)
{
    streamoo_encoder_demo_t *d = context;
    if (d->audio_post) return d->audio_post(d->audio_context,cmd);
#if SYNTHETIC_UI
    if (d->control_pending) return false;
    d->control = *cmd;
    d->control_pending = true;
    return true;
#else
    return false;
#endif
}
bool streamoo_encoder_demo_init(streamoo_encoder_demo_t *d)
{
    if (!d) return false;
#if SYNTHETIC_UI
    *d = (streamoo_encoder_demo_t){.station_id=1002, .generation=1, .volume=50, .running=true};
    if (!streamoo_ui_init(&d->ui, 160, select_station, d) ||
        !streamoo_ui_set_catalog(&d->ui, stations, 5, 1)) return false;
    streamoo_ui_set_playing(&d->ui, d->station_id);
#else
    *d = (streamoo_encoder_demo_t){.generation=1,.volume=50,.imported_catalog=true};
    if(!streamoo_ui_init(&d->ui,160,select_station,d) ||
       !streamoo_ui_set_catalog(&d->ui,NULL,0,1))return false;
    d->ui.running=false;
#endif
    return true;
}
bool streamoo_encoder_demo_input(streamoo_encoder_demo_t *d, streamoo_input_event_t e,
                                  streamoo_control_mode_t mode, uint64_t now)
{
    if (!d || !streamoo_ui_handle_input(&d->ui, e, mode, post_control, d, now)) return false;
#if SYNTHETIC_UI
    /* Diagnostic owner applies copied commands after the UI callback returns.
     * Production adapter will enqueue to player_manager instead. */
    if (d->selection_pending) {
        if (d->selection.catalog_generation == d->ui.catalog_generation) {
            d->station_id = d->selection.station_id;d->running=true;
            ++d->generation;
        }
        d->selection_pending = false;
    }
    if (d->control_pending) {
        if (d->control.kind == STREAMOO_CONTROL_TOGGLE) d->running = !d->running;
        else if(d->control.kind==STREAMOO_CONTROL_PLAY)d->running=true;
        else if(d->control.kind==STREAMOO_CONTROL_STOP)d->running=false;
        else {
            int64_t volume = (int64_t)d->volume + d->control.delta;
            d->volume = volume < 0 ? 0 : volume > 100 ? 100 : (int)volume;
        }
        d->control_pending = false;
    }
#endif
    d->ui.running=d->running;
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
    for (uint32_t i=0; i<d->ui.count; ++i)
        if (d->ui.stations[i].stable_id == d->station_id)
            s.station_name=(streamoo_ui_text_t){d->ui.stations[i].name,d->ui.stations[i].name_bytes};
#if SYNTHETIC_UI
    static const char silent[]="TEST sterowania - bez audio";
    static const char audible[]="TEST: ton 1kHz, widżety TEST";
    s.title=d->audio_post ? (streamoo_ui_text_t){(const uint8_t *)audible,sizeof audible-1} :
                            (streamoo_ui_text_t){(const uint8_t *)silent,sizeof silent-1};
    if (d->audio_post && d->audio_title.data) s.title=d->audio_title;
#endif
    if (d->imported_catalog) {
        static const uint8_t choose[]="Wybierz stację z listy", selected[]="Odtwarzanie niedostępne", empty[]="Wgraj playlistę przez WWW";
        s.visualization_valid=false;
        s.title=!d->ui.count ? (streamoo_ui_text_t){empty,sizeof empty-1} :
            d->station_id ? (streamoo_ui_text_t){selected,sizeof selected-1} :
            (streamoo_ui_text_t){choose,sizeof choose-1};
        if(d->audio_post&&d->audio_title.bytes)s.title=d->audio_title;
    }
    s.clock_valid=d->clock_valid;
    s.clock=d->clock;
    s.wifi_connected=d->wifi_connected;
    s.rssi_valid=d->rssi_valid;
    s.rssi_dbm=d->rssi_dbm;
    s.audio_info_valid=d->codec[0]!=0;
    s.bitrate_kbps=d->codec[0] ? d->bitrate_kbps:0;
    s.bitrate_measured=d->codec[0] && d->bitrate_measured;
    s.codec=(streamoo_ui_text_t){(const uint8_t *)d->codec,strlen(d->codec)};
#if SYNTHETIC_UI
    for (unsigned i=0; i<24; ++i) {
        unsigned phase=(unsigned)(now%2000+i*61)%2000;
        s.visualization.bands[i]=d->running ? (phase<1000 ? phase : 2000-phase)*32 : 0;
    }
    s.visualization.rms[0]=s.visualization.bands[4];
    s.visualization.rms[1]=s.visualization.bands[12];
    s.visualization.peak[0]=s.visualization.rms[0];
    s.visualization.peak[1]=s.visualization.rms[1];
#endif
    if (d->imported_catalog && d->levels_valid &&
        (!streamoo_visualization_is_spectrum(d->ui.visualization) || d->levels.band_count)) {
        s.visualization = d->levels;
        s.visualization_valid = true;
    }
    if (!streamoo_ui_update_home(&d->ui,&s,now)) return false;
    streamoo_canvas_t canvas;
    if (!streamoo_canvas_init(&canvas,frame,bytes,256,64,128)) return false;
    if (d->ui.screen == STREAMOO_UI_STATIONS) {
        streamoo_canvas_fill(&canvas,(streamoo_rect_t){0,0,256,64},0);
        const char *label="Lista stacji";
#if SYNTHETIC_UI
        if(!d->imported_catalog)label="TEST - lista stacji";
#endif
        return streamoo_text_line(&canvas,(streamoo_rect_t){0,0,256,8},(const uint8_t *)label,strlen(label),
            STREAMOO_TEXT_CENTER,0,0,0,10,streamoo_font5x8,NULL) &&
            streamoo_ui_draw_list(&d->ui,&canvas,(streamoo_rect_t){0,8,256,56},now);
    }
    if (!streamoo_ui_draw_home(&d->ui,&canvas,now)) return false;
    if(d->network_ip[0]) return streamoo_ui_connection_row_draw(&canvas,
        (const uint8_t *)d->network_ip,strlen(d->network_ip),d->volume);
#if SYNTHETIC_UI
    char status[44];
    int length=snprintf(status,sizeof(status),"TEST VOL %d  %s  W%u",d->volume,
                        d->running ? (d->buffering ? "BUF" : "PLAY") : "STOP",(unsigned)d->ui.visualization+1);
    if (length<0 || (size_t)length>=sizeof(status)) return false;
    /* Third HOME row shows the LAN address when connected, otherwise test status. */
    streamoo_canvas_fill(&canvas,(streamoo_rect_t){0,24,256,8},0);
    return streamoo_text_line(&canvas,(streamoo_rect_t){0,24,256,8},
        (const uint8_t *)status,(size_t)length,STREAMOO_TEXT_CENTER,0,0,0,12,streamoo_font5x8,NULL);
#else
    static const uint8_t no_address[]="0.0.0.0";
    return streamoo_ui_connection_row_draw(&canvas,no_address,sizeof no_address-1,d->volume);
#endif
}

bool streamoo_encoder_demo_bind_audio(streamoo_encoder_demo_t *d, streamoo_control_post_fn post, void *context)
{
    if (!d || !d->ui.post || !post || d->control_pending) return false;
    d->audio_post=post; d->audio_context=context;
    return true;
}
bool streamoo_encoder_demo_audio_state(streamoo_encoder_demo_t *d, uint8_t volume, bool running)
{
    if (!d || !d->audio_post || volume>100) return false;
    if(d->running&&!running)d->ui.screen=STREAMOO_UI_HOME;
    d->volume=volume; d->running=running;d->ui.running=running;
    return true;
}

bool streamoo_encoder_demo_set_ip(streamoo_encoder_demo_t *d, const char *ip)
{
    if (!d) return false;
    char copy[16]={0};
    if (ip) {
        size_t i=0;
        while (i<sizeof copy && ip[i]) {
            if ((ip[i]<'0' || ip[i]>'9') && ip[i]!='.') return false;
            copy[i]=ip[i]; ++i;
        }
        if (i==sizeof copy) return false;
    }
    memcpy(d->network_ip,copy,sizeof copy);
    return true;
}

bool streamoo_encoder_demo_set_catalog(streamoo_encoder_demo_t *d,const streamoo_ui_station_t *stations,uint32_t count,uint64_t generation)
{
    if (!d || !streamoo_ui_set_catalog(&d->ui,stations,count,generation)) return false;
    bool retained=false;
    for(uint32_t i=0;i<count;++i) if(stations[i].stable_id==d->station_id) retained=true;
    if(!retained) {d->station_id=0;d->running=false;}
    d->selection_pending=false;d->ui.running=d->running;
    ++d->generation;
    d->imported_catalog=true;
    /* Old snapshot may be released immediately after this call. */
    d->ui.home=(streamoo_home_snapshot_t){0};
    d->ui.home_received=false;
    streamoo_ui_set_playing(&d->ui,d->station_id);
    return true;
}

void streamoo_encoder_demo_bind_station_owner(streamoo_encoder_demo_t *d,streamoo_ui_post_fn post,void *context)
{ if(d){d->station_post=post;d->station_context=context;} }

bool streamoo_encoder_demo_levels(streamoo_encoder_demo_t *d,
                                  const streamoo_visualization_model_t *model)
{
    if (!d) return false;
    if (model) {
        if(model->band_count>STREAMOO_VIS_MAX_BANDS)return false;
        for(unsigned b=0;b<model->band_count;++b)
            if(model->bands[b]>32767 || (b<STREAMOO_VIS_PEAK_BANDS && model->band_peaks[b]>32767))return false;
        for (unsigned ch=0; ch<2; ++ch)
            if (model->rms[ch]>32767 || model->peak[ch]>32767) return false;
    }
    d->levels_valid=model!=NULL;
    d->levels=model ? *model : (streamoo_visualization_model_t){0};
    return true;
}

bool streamoo_encoder_demo_technical(streamoo_encoder_demo_t *d,bool connected,
                                    bool valid,int dbm,const char *codec)
{
    if(!d || (connected && valid && (dbm < -127 || dbm > 0)))return false;
    char copy[8]={0};
    if(codec) {
        unsigned n=0;
        for(;n<sizeof copy && codec[n];++n) {
            if((unsigned char)codec[n]<32 || (unsigned char)codec[n]>126)return false;
            copy[n]=codec[n];
        }
        if(n==sizeof copy)return false;
    }
    d->wifi_connected=connected;
    d->rssi_valid=connected&&valid;
    d->rssi_dbm=d->rssi_valid ? dbm:0;
    memcpy(d->codec,copy,sizeof copy);
    return true;
}
