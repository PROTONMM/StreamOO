idf_component_register(SRCS "streamoo_clock.c" "streamoo_clock_settings.c" INCLUDE_DIRS "include"
    REQUIRES esp_common PRIV_REQUIRES nvs_flash freertos)
