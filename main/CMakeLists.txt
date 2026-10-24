idf_component_register(SRCS "main.c" "display_smoke.c" INCLUDE_DIRS "."
                       REQUIRES board_config platform hardware_probe ssd1322
                                heap freertos esp_timer log)
