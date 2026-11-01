idf_component_register(SRCS "streamoo_flac_header.c" "streamoo_flac_parse.c" "streamoo_decoder.c" "streamoo_decode_stream.c" INCLUDE_DIRS "include"
                       REQUIRES esp_audio_codec pcm_adapter compressed_source PRIV_REQUIRES log)
