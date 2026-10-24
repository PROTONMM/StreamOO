idf_component_register(SRCS "ssd1322_protocol.c" "streamoo_ssd1322.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_common esp_driver_gpio esp_driver_spi heap freertos)
