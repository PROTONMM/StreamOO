idf_component_register(SRCS "streamoo_preferences.c" "streamoo_preferences_policy.c"
                       INCLUDE_DIRS "include" REQUIRES nvs_flash esp_timer freertos log)
