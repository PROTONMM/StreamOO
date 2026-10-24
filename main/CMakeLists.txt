idf_component_register(SRCS "main.c" "display_smoke.c" "canvas_smoke.c" "text_smoke.c"
                           "text_layout_smoke.c" "visualization_probe.c" INCLUDE_DIRS "."
                       REQUIRES board_config platform hardware_probe ssd1322
                                graphics text ui_widgets heap freertos esp_timer log)
