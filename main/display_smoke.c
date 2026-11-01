#include "display_smoke.h"
#include "encoder_demo.h"
#include "streamoo_settings.h"
#include "streamoo_ui_ap.h"
#include "streamoo_ui_boot.h"
#include "streamoo_network.h"
#include "streamoo_config_store.h"
#include "streamoo_player.h"
#include "streamoo_clock_settings.h"
#include <time.h>
_Static_assert(sizeof(time_t)>=8,"Clock requires 64-bit time_t through2099");
#include "audio_control_probe.h"
#include "streamoo_preferences.h"
#include "streamoo_input_probe.h"
#include "canvas_smoke.h"
#include "text_smoke.h"
#include "home_probe.h"
#include "ui_list_probe.h"
#include "display_timing_probe.h"

#include <inttypes.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "streamoo_ssd1322.h"

static bool boot_logo_presented;

#ifdef CONFIG_STREAMOO_LCD_TIMING_PROBE
static esp_err_t display_timing_probe(streamoo_ssd1322_t *display, uint8_t *frame)
{
    static const char *const names[] = {
        "HOLD no SPI after first frame", "REPEAT identical frame 25fps",
        "LOAD changing lit area 25fps", "LOAD changing lit area 10fps",
        "MOVE constant lit area 25fps"
    };
    for (unsigned phase = 0; phase < STREAMOO_OLED_PROBE_COUNT; ++phase) {
        ESP_LOGI("oled_probe", "phase=%u/5 %s; observe fixed upper text/gray fields for 8s", phase + 1, names[phase]);
        const int64_t start = esp_timer_get_time();
        TickType_t wake = xTaskGetTickCount();
        const unsigned period_ms = phase == STREAMOO_OLED_LOAD10 ? 100 : 40;
        unsigned frames = 0;
        int64_t total_us = 0, max_us = 0;
        while (esp_timer_get_time() - start < 8000000) {
            streamoo_input_event_t input;
            if (streamoo_input_probe_receive(&input)) {
                ESP_LOGI("oled_probe", "Aborted by encoder; returning to normal UI");
                return ESP_OK;
            }
            if (phase != STREAMOO_OLED_HOLD || frames == 0) {
                uint32_t elapsed = (esp_timer_get_time() - start) / 1000;
                if (!streamoo_oled_probe_draw(frame, SSD1322_FRAME_BYTES, phase, elapsed)) return ESP_ERR_INVALID_ARG;
                int64_t before = esp_timer_get_time();
                esp_err_t error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
                int64_t duration = esp_timer_get_time() - before;
                if (error != ESP_OK) return error;
                ++frames; total_us += duration;
                if (duration > max_us) max_us = duration;
            }
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(period_ms) ? pdMS_TO_TICKS(period_ms) : 1);
        }
        ESP_LOGI("oled_probe", "phase=%u frames=%u elapsed_us=%" PRId64 " present_avg_us=%" PRId64 " present_max_us=%" PRId64,
                 phase + 1, frames, esp_timer_get_time() - start, frames ? total_us / frames : 0, max_us);
    }
    ESP_LOGI("oled_probe", "Complete; returning to normal UI");
    return ESP_OK;
}
#endif

#ifdef CONFIG_STREAMOO_UI_ENABLED
static bool sync_catalog(streamoo_encoder_demo_t *demo,streamoo_catalog_snapshot_t **held,streamoo_ui_station_t **entries)
{
    streamoo_catalog_snapshot_t *next=streamoo_config_catalog_acquire();
    if (!next || next==*held) {streamoo_config_catalog_release(next);return true;}
    streamoo_catalog_view_t view=streamoo_config_catalog_view(next);
    streamoo_ui_station_t *items=view.count ? heap_caps_calloc(view.count,sizeof *items,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT) : NULL;
    if(view.count&&!items) {streamoo_config_catalog_release(next);return false;}
    for(uint32_t i=0;i<view.count;++i) items[i]=(streamoo_ui_station_t){view.stations[i].stable_id,view.stations[i].name,view.stations[i].name_bytes};
    if(!streamoo_encoder_demo_set_catalog(demo,items,view.count,view.generation)) {
        heap_caps_free(items);streamoo_config_catalog_release(next);return false;
    }
    streamoo_config_catalog_release(*held);heap_caps_free(*entries);
    *held=next;*entries=items;
    ESP_LOGI("display_smoke","Station catalog rows=%" PRIu32 " generation=%" PRIu64,view.count,view.generation);
    return true;
}
#endif

static void display_task(void *context)
{
    (void)context;
    const streamoo_ssd1322_config_t config = {
        .spi_host = CONFIG_STREAMOO_LCD_SPI_HOST,
        .mosi = CONFIG_STREAMOO_LCD_MOSI, .sclk = CONFIG_STREAMOO_LCD_SCLK,
        .cs = CONFIG_STREAMOO_LCD_CS, .dc = CONFIG_STREAMOO_LCD_DC,
        .reset = CONFIG_STREAMOO_LCD_RST,
        .clock_hz = CONFIG_STREAMOO_LCD_SPI_CLOCK_MHZ * 1000000,
        .panel = {.column_offset = CONFIG_STREAMOO_LCD_COLUMN_OFFSET,
                  .remap = CONFIG_STREAMOO_LCD_REMAP,
                  .contrast = 0x7f, .master_current = 3},
    };
    streamoo_ssd1322_t *display = NULL;
    uint8_t *frame = heap_caps_calloc(1, SSD1322_FRAME_BYTES,
                                     MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    esp_err_t error = frame ? streamoo_ssd1322_open(&config, &display) : ESP_ERR_NO_MEM;
    const bool show_logo=!boot_logo_presented;
    if(error==ESP_OK&&show_logo&&!streamoo_ui_boot_draw(frame,SSD1322_FRAME_BYTES))error=ESP_ERR_INVALID_STATE;
    if (error == ESP_OK) error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
    if (error == ESP_OK) error = streamoo_ssd1322_enable(display, true);
    if(error==ESP_OK&&show_logo){boot_logo_presented=true;vTaskDelay(pdMS_TO_TICKS(1500));}
#ifdef CONFIG_STREAMOO_UI_ENABLED
    if (error == ESP_OK) {
        streamoo_catalog_snapshot_t *catalog_snapshot=NULL;
        streamoo_ui_station_t *catalog_entries=NULL;
        uint64_t catalog_check_at=0;
        streamoo_encoder_demo_t demo;
        if (!streamoo_encoder_demo_init(&demo)) error = ESP_ERR_INVALID_STATE;
        streamoo_preferences_value_t restored = {50,0};
        const bool preferences_ready = streamoo_preferences_initial(&restored);
        demo.volume = restored.volume;
        if (streamoo_network_boot_ap_enabled()) {
            if(!streamoo_encoder_demo_set_catalog(&demo,NULL,0,0) ||
               !streamoo_encoder_demo_bind_audio(&demo,streamoo_player_control,NULL))error=ESP_ERR_INVALID_STATE;
            streamoo_encoder_demo_bind_station_owner(&demo,streamoo_player_select,NULL);
        }
        bool panel_enabled=true;
        bool startup_logo=show_logo;
        uint64_t startup_connected_at=0;
        streamoo_player_phase_t startup_phase=STREAMOO_PLAYER_STOPPED;
        bool startup_player_ready=false;
#ifdef CONFIG_STREAMOO_LCD_TIMING_PROBE
        bool timing_probe_pending=true;
#endif
        uint64_t player_generation=0;
        uint32_t player_volume_revision=0;
        if (!streamoo_ui_set_visualization(&demo.ui, restored.visualization)) error = ESP_ERR_INVALID_STATE;
#ifdef CONFIG_STREAMOO_AUDIO_CONTROL_TEST
        if (!streamoo_network_boot_ap_enabled() &&
            !streamoo_encoder_demo_bind_audio(&demo,streamoo_audio_control_post,NULL)) error=ESP_ERR_INVALID_STATE;
        streamoo_encoder_demo_audio_state(&demo,restored.volume,false);
#ifdef CONFIG_STREAMOO_MP3_FIXTURE_TEST
        static const uint8_t mp3_title[]="TEST: plik MP3, widżety TEST";
        if(!streamoo_network_boot_ap_enabled()) demo.audio_title=(streamoo_ui_text_t){mp3_title,sizeof mp3_title-1};
#elif defined(CONFIG_STREAMOO_AAC_FIXTURE_TEST)
        static const uint8_t aac_title[]="TEST: AAC-LC, widżety TEST";
        if(!streamoo_network_boot_ap_enabled()) demo.audio_title=(streamoo_ui_text_t){aac_title,sizeof aac_title-1};
#endif
        uint32_t volume_revision=0;
#endif
        streamoo_input_event_t event;
        /* Discard input captured before this UI session was ready. */
        while (streamoo_input_probe_receive(&event)) {}
        TickType_t wake = xTaskGetTickCount();
#ifdef CONFIG_STREAMOO_DIAGNOSTICS
        int64_t oled_stats_at=esp_timer_get_time(),oled_max_present=0;
        uint32_t oled_checked=0,oled_sent=0,oled_rows=0;
        uint64_t oled_bytes=0;
#endif
#ifdef CONFIG_STREAMOO_DIAGNOSTICS
        ESP_LOGI("display_smoke", "%s",streamoo_network_boot_ap_enabled() ?
            "Encoder UI ready; stored station catalog; network player" :
            "Interactive encoder TEST ready; synthetic catalog/widgets; no stream player");
#endif
        while (error == ESP_OK) {
            uint64_t now = (uint64_t)esp_timer_get_time() / 1000;
            if(streamoo_network_boot_ap_enabled() && now>=catalog_check_at) {
                if(!sync_catalog(&demo,&catalog_snapshot,&catalog_entries))
                    ESP_LOGW("display_smoke","Catalog update deferred; previous snapshot retained");
                catalog_check_at=now+1000;
            }
#ifdef CONFIG_STREAMOO_AUDIO_CONTROL_TEST
            streamoo_audio_control_state_t audio;
            if (!streamoo_network_boot_ap_enabled() && streamoo_audio_control_state(&audio)) {
                streamoo_encoder_demo_audio_state(&demo,audio.model.volume,audio.model.running);
                demo.buffering=audio.buffering;
                if (preferences_ready && volume_revision!=audio.model.volume_revision) {
                    const streamoo_preferences_value_t changed={audio.model.volume,(uint8_t)demo.ui.visualization};
                    if (streamoo_preferences_submit_changed(changed)) volume_revision=audio.model.volume_revision;
                }
            }
#endif
            char live_codec[8]={0};
            demo.bitrate_kbps=0;
            demo.bitrate_measured=false;
            if(streamoo_network_boot_ap_enabled()) {
                streamoo_player_spectrum_enabled(demo.ui.screen==STREAMOO_UI_HOME &&
                    demo.ui.visualization==STREAMOO_VIS_SPECTRUM);
                streamoo_player_state_t player;
                if(streamoo_player_state(&player)) {
                    startup_phase=player.phase;
                    startup_player_ready=player.ready;
                    memcpy(live_codec,player.codec,sizeof live_codec);
                    demo.bitrate_kbps=player.bitrate_kbps;
                    demo.bitrate_measured=player.bitrate_measured;
                    if(demo.station_id!=player.station_id||player_generation!=player.generation){
                        demo.station_id=player.station_id;++demo.generation;
                        streamoo_ui_set_playing(&demo.ui,demo.station_id);player_generation=player.generation;
                    }
                    streamoo_encoder_demo_audio_state(&demo,player.volume,player.running);
                    demo.buffering=player.phase!=STREAMOO_PLAYER_PLAYING;
                    streamoo_visualization_model_t levels = {0};
                    for (unsigned ch=0;ch<2;++ch) {
                        levels.rms[ch]=player.metering.levels.rms[ch];
                        levels.peak[ch]=player.metering.levels.peak[ch];
                    }
                    if(player.metering.spectrum_valid) {
                        levels.band_count=24;
                        memcpy(levels.bands,player.metering.bands,sizeof player.metering.bands);
                    }
                    bool visualization_valid=demo.ui.visualization==STREAMOO_VIS_SPECTRUM ?
                        player.metering.spectrum_valid : player.metering.levels.valid;
                    streamoo_encoder_demo_levels(&demo,
                        player.phase==STREAMOO_PLAYER_PLAYING &&
                        player.metering.generation==player.generation &&
                        visualization_valid ? &levels : NULL);
                    const char *message=player.title[0]?player.title:
                        player.phase==STREAMOO_PLAYER_PLAYING?"Odtwarzanie":
                        player.phase==STREAMOO_PLAYER_BUFFERING?"Buforowanie...":
                        player.phase==STREAMOO_PLAYER_CONNECTING?"Łączenie ze stacją...":
                        player.phase==STREAMOO_PLAYER_RETRY?"Ponawianie połączenia...":
                        player.phase==STREAMOO_PLAYER_WAIT_NETWORK?"Brak połączenia Wi-Fi":
                        player.station_id?"Zatrzymano":"Wybierz stację z listy";
                    size_t length=strlen(message);if(strcmp(demo.player_text,message))++demo.generation;
                    memcpy(demo.player_text,message,length+1);
                    demo.audio_title=(streamoo_ui_text_t){(const uint8_t *)demo.player_text,length};
                    if(preferences_ready&&player_volume_revision!=player.volume_revision){
                        const streamoo_preferences_value_t changed={player.volume,(uint8_t)demo.ui.visualization};
                        if(streamoo_preferences_submit_changed(changed))player_volume_revision=player.volume_revision;
                    }
                }
            }
            streamoo_network_ap_display_t ap;
            bool ap_active=streamoo_network_ap_display(&ap);
            streamoo_network_state_t network={0};
            streamoo_network_state(&network);
            streamoo_clock_civil_t civil_time;
            demo.clock_valid=streamoo_clock_project((int64_t)time(NULL),network.time_synced,
                streamoo_clock_settings_get(),&civil_time);
            demo.clock=(streamoo_ui_clock_t){civil_time.year,civil_time.month,civil_time.day,
                civil_time.hour,civil_time.minute,civil_time.second};
            streamoo_encoder_demo_technical(&demo,network.sta_connected,
                network.rssi_valid,network.rssi_dbm,live_codec);
            const bool setup=streamoo_network_boot_ap_enabled() && (ap_active || !network.sta_connected);
            if (!streamoo_encoder_demo_set_ip(&demo,network.sta_connected ? network.sta_ip : NULL)) {
                error=ESP_ERR_INVALID_ARG; break;
            }
            if (ap_active && demo.ui.screen==STREAMOO_UI_STATIONS) streamoo_ui_cancel(&demo.ui);
            for (unsigned n=0; n<32 && streamoo_input_probe_receive(&event); ++n) {
#ifdef CONFIG_STREAMOO_ENCODER_MODE_B
                const streamoo_control_mode_t mode = STREAMOO_CONTROL_MODE_B;
#else
                const streamoo_control_mode_t mode = STREAMOO_CONTROL_MODE_A;
#endif
                if (setup) continue;
                startup_logo=false;
                if (!streamoo_encoder_demo_input(&demo,event,mode,now))
                    ESP_LOGW("display_smoke", "Gesture rejected; state preserved");
                const streamoo_preferences_value_t preferences = {(uint8_t)demo.volume, (uint8_t)demo.ui.visualization};
                if (preferences_ready && !streamoo_preferences_submit(preferences))
                    ESP_LOGW("display_smoke", "Preferences update not accepted");
            }
            streamoo_settings_t settings=streamoo_settings_get();
            if(!setup&&!streamoo_ui_list_timeout(&demo.ui,now,settings.list_timeout_s,settings.list_autoselect))
                ESP_LOGW("display_smoke","List timeout deferred: command queue busy");
            /* Hold the already presented boot frame; never repeat on reconnect.
             * Bound station startup so a failed source cannot hide diagnostics. */
            if(startup_logo && streamoo_network_boot_ap_enabled()) {
                if(network.sta_connected && !startup_connected_at) startup_connected_at=now;
                bool pending=!startup_player_ready || !catalog_snapshot ||
                    (demo.station_id && demo.running && startup_phase!=STREAMOO_PLAYER_PLAYING &&
                     startup_phase!=STREAMOO_PLAYER_RETRY);
                bool waiting=streamoo_ui_boot_wait(ap_active,network.sta_connected,pending,
                    now-startup_connected_at);
                if(waiting) {
                    memset(&ap,0,sizeof ap);
                    vTaskDelayUntil(&wake,pdMS_TO_TICKS(40));continue;
                }
                startup_logo=false;
            }
            bool visible=setup||!demo.station_id||demo.running;
            if(!visible){
                if(panel_enabled){error=streamoo_ssd1322_enable(display,false);if(error==ESP_OK)panel_enabled=false;}
                vTaskDelayUntil(&wake,pdMS_TO_TICKS(40));continue;
            }
#ifdef CONFIG_STREAMOO_LCD_TIMING_PROBE
            if(timing_probe_pending && panel_enabled && !setup && startup_phase==STREAMOO_PLAYER_PLAYING) {
                timing_probe_pending=false;
                error=display_timing_probe(display,frame);
                wake=xTaskGetTickCount();
                oled_stats_at=esp_timer_get_time();oled_max_present=0;
                oled_checked=oled_sent=oled_rows=0;oled_bytes=0;
                continue;
            }
#endif
            bool drawn=ap_active ? streamoo_ui_ap_draw(frame,SSD1322_FRAME_BYTES,
                (const uint8_t *)ap.ssid,15,ap.password,ap.password_bytes) :
                setup ? streamoo_ui_sta_draw(frame,SSD1322_FRAME_BYTES,
                    (const uint8_t *)network.sta_ssid,strlen(network.sta_ssid),
                    (const uint8_t *)network.sta_ip,network.sta_connected?strlen(network.sta_ip):0) :
                streamoo_encoder_demo_draw(&demo,frame,SSD1322_FRAME_BYTES,now);
            memset(&ap,0,sizeof ap);
            if (!drawn) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            ssd1322_update_t update={0};
#ifdef CONFIG_STREAMOO_DIAGNOSTICS
            int64_t present_at=esp_timer_get_time();
#endif
            if(!panel_enabled) {
                /* A wake always restores all RAM before enabling the panel. */
                error=streamoo_ssd1322_present(display,frame,SSD1322_FRAME_BYTES);
                update=(ssd1322_update_t){SSD1322_FRAME_BYTES,0,SSD1322_HEIGHT};
            } else error=streamoo_ssd1322_present_changed(display,frame,SSD1322_FRAME_BYTES,&update);
#ifdef CONFIG_STREAMOO_DIAGNOSTICS
            int64_t measured_at=esp_timer_get_time(),present_us=measured_at-present_at;
            if(error==ESP_OK) {
                ++oled_checked;
                if(update.bytes){++oled_sent;oled_rows+=update.row_count;oled_bytes+=update.bytes;}
                if(present_us>oled_max_present)oled_max_present=present_us;
            }
            if(measured_at-oled_stats_at>=10000000) {
                ESP_LOGI("display_smoke","OLED UI interval_ms=%" PRId64 " checked=%" PRIu32
                    " sent=%" PRIu32 " rows=%" PRIu32 " payload=%" PRIu64 " max_present_us=%" PRId64,
                    (measured_at-oled_stats_at)/1000,oled_checked,oled_sent,oled_rows,oled_bytes,oled_max_present);
                oled_stats_at=measured_at;oled_max_present=0;oled_checked=oled_sent=oled_rows=0;oled_bytes=0;
            }
#endif
            if(error==ESP_OK&&!panel_enabled){error=streamoo_ssd1322_enable(display,true);if(error==ESP_OK)panel_enabled=true;}
            vTaskDelayUntil(&wake,pdMS_TO_TICKS(40) ? pdMS_TO_TICKS(40) : 1);
        }
        streamoo_config_catalog_release(catalog_snapshot);
        heap_caps_free(catalog_entries);
    }
#elif defined(CONFIG_STREAMOO_DISPLAY_TEST)
    if (error == ESP_OK) {
        ESP_LOGW("display_smoke", "Experimental panel profile; verify image on OLED. RESET GPIO=%d",
                 CONFIG_STREAMOO_LCD_RST);
        /* Finite test avoids leaving a bright diagnostic image indefinitely. */
        for (int step = 0; step < 6 && error == ESP_OK; ++step) {
            if (step == 0) memset(frame, 0x00, SSD1322_FRAME_BYTES);
            if (step == 1) memset(frame, 0xff, SSD1322_FRAME_BYTES);
            if (step == 2) ssd1322_test_pattern(frame, SSD1322_FRAME_BYTES);
            if (step == 3) {
                for (size_t i = 0; i < SSD1322_FRAME_BYTES; ++i) frame[i] ^= 0xff;
            }
            if (step == 4) {
                const int64_t render_start = esp_timer_get_time();
                if (!streamoo_canvas_smoke(frame, SSD1322_FRAME_BYTES)) {
                    error = ESP_ERR_INVALID_ARG;
                    break;
                }
                ESP_LOGI("display_smoke", "canvas scene raster=%" PRId64 " us",
                         esp_timer_get_time() - render_start);
            }
            if (step == 5 && !streamoo_text_smoke(frame, SSD1322_FRAME_BYTES)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            const int64_t start = esp_timer_get_time();
            error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
            ESP_LOGI("display_smoke", "step=%d frame transfer=%" PRId64 " us, result=%s",
                     step, esp_timer_get_time() - start, esp_err_to_name(error));
            if (error == ESP_OK) vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }
    if (error == ESP_OK) {
        streamoo_home_probe_t home;
        if (!streamoo_home_probe_init(&home)) error = ESP_ERR_INVALID_STATE;
        const int64_t animation_start = esp_timer_get_time();
        TickType_t wake = xTaskGetTickCount();
        unsigned frames = 0;
        while (error == ESP_OK && esp_timer_get_time() - animation_start < 20000000) {
            const uint64_t elapsed_ms = (esp_timer_get_time() - animation_start) / 1000;
            if (!streamoo_home_probe_draw(&home, frame, SSD1322_FRAME_BYTES, elapsed_ms)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
            ++frames;
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(40) ? pdMS_TO_TICKS(40) : 1);
        }
        ESP_LOGI("display_smoke", "marquee frames=%u elapsed=%" PRId64 " us",
                 frames, esp_timer_get_time() - animation_start);
    }
    if (error == ESP_OK) {
        streamoo_ui_list_probe_t probe;
        if (!streamoo_ui_list_probe_init(&probe)) error = ESP_ERR_INVALID_STATE;
        const int64_t list_start = esp_timer_get_time();
        TickType_t wake = xTaskGetTickCount();
        while (error == ESP_OK && esp_timer_get_time() - list_start < 8000000) {
            const uint64_t elapsed = (esp_timer_get_time() - list_start) / 1000;
            if (!streamoo_ui_list_probe_draw(&probe, frame, SSD1322_FRAME_BYTES, elapsed)) {
                error = ESP_ERR_INVALID_ARG;
                break;
            }
            error = streamoo_ssd1322_present(display, frame, SSD1322_FRAME_BYTES);
            vTaskDelayUntil(&wake, pdMS_TO_TICKS(40) ? pdMS_TO_TICKS(40) : 1);
        }
        if (error == ESP_OK)
            ESP_LOGI("display_smoke", "list diagnostic requested=%" PRIu64 " snapshot=%" PRIu64,
                     probe.requested_id, probe.ui.playing_id);
    }
#endif
    if (error != ESP_OK) ESP_LOGE("display_smoke", "Failed: %s", esp_err_to_name(error));
    if (display) {
        const esp_err_t close_error = streamoo_ssd1322_close(display);
        if (close_error != ESP_OK) ESP_LOGE("display_smoke", "Close failed: %s", esp_err_to_name(close_error));
    }
    heap_caps_free(frame);
#ifdef CONFIG_STREAMOO_DISPLAY_TEST
    ESP_LOGI("display_smoke", "Test ended; visual acceptance remains HARDWARE_TEST_PENDING");
#endif
    vTaskDelete(NULL);
}

esp_err_t streamoo_display_smoke_start(void)
{
    return xTaskCreate(display_task, "display_smoke", 4096, NULL, 2, NULL) == pdPASS
               ? ESP_OK : ESP_ERR_NO_MEM;
}
