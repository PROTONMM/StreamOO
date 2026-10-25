idf_component_register(SRCS "main.c" "mp3_fixture.c" "audio_control_model.c" "audio_control_probe.c" "audio_probe.c" "audio_tone.c" "display_smoke.c" "canvas_smoke.c" "text_smoke.c"
                           "encoder_demo.c" "home_probe.c" "ui_list_probe.c" INCLUDE_DIRS "."
                       REQUIRES decoder pcm_adapter pcm audio_gain audio_output preferences board_config platform hardware_probe ssd1322
                                graphics text ui_widgets ui heap freertos esp_timer log)

if(CONFIG_STREAMOO_MP3_FIXTURE_TEST)
    target_add_binary_data(${COMPONENT_LIB} "${PROJECT_DIR}/assets/audio/channels_48k_stereo.mp3" BINARY)
endif()
