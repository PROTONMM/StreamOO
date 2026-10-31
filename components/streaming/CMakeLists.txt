idf_component_register(SRCS "streamoo_hls.c" "streamoo_playlist_resolver.c" "streamoo_icy.c" "streamoo_live_decode.c" "streamoo_http_stream.c"
    INCLUDE_DIRS "include" REQUIRES decoder compressed_source
    PRIV_REQUIRES esp_http_client mbedtls heap freertos log)
