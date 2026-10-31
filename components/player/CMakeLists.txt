idf_component_register(SRCS "streamoo_player.c" INCLUDE_DIRS "include"
    REQUIRES ui esp_common
    PRIV_REQUIRES configuration network streaming audio_output pcm audio_gain heap freertos esp_timer log board_config)
