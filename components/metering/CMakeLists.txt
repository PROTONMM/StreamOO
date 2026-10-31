idf_component_register(SRCS "streamoo_metering.c" INCLUDE_DIRS "include"
    REQUIRES levels pcm esp_common
    PRIV_REQUIRES heap esp_timer freertos)
