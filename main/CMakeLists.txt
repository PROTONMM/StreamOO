idf_component_register(SRCS "main.c" "display_smoke.c" "canvas_smoke.c" "text_smoke.c"
                           "text_layout_smoke.c" INCLUDE_DIRS "."
                       REQUIRES board_config platform hardware_probe ssd1322
                                graphics text heap freertos esp_timer log)
