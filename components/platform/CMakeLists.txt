idf_component_register(SRCS "streamoo_platform.c" "streamoo_crypto_prepare.c" INCLUDE_DIRS "include"
                       REQUIRES esp_common esp_psram esp_hw_support spi_flash
                                heap freertos log esp_timer
                       PRIV_REQUIRES mbedtls)
