idf_component_register(SRCS "main.c" "audio_probe.c" "audio_tone.c" "display_smoke.c" "canvas_smoke.c" "text_smoke.c"
                           "encoder_demo.c" "home_probe.c" "ui_list_probe.c" INCLUDE_DIRS "."
                       REQUIRES audio_output preferences board_config platform hardware_probe ssd1322
                                graphics text ui_widgets ui heap freertos esp_timer log)
