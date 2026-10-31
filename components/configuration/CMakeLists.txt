idf_component_register(SRCS "streamoo_config_model.c" "streamoo_config_store.c" INCLUDE_DIRS "include"
                       REQUIRES esp_common stations PRIV_REQUIRES espressif__cjson heap nvs_flash esp_partition joltwallet__littlefs freertos)
