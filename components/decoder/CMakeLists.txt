idf_component_register(SRCS "streamoo_mp3_decoder.c" INCLUDE_DIRS "include"
                       REQUIRES esp_audio_codec pcm_adapter PRIV_REQUIRES log)
