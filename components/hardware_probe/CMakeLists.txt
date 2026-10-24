idf_component_register(SRCS "streamoo_input_probe.c" INCLUDE_DIRS "include"
                       REQUIRES input esp_timer esp_common esp_driver_gpio freertos log)
