idf_component_register(SRCS "streamoo_audio_output.c" INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2s esp_hw_support esp_timer heap freertos)
