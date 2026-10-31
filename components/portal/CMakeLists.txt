idf_component_register(SRCS "streamoo_portal_config.c" "streamoo_portal.c" "streamoo_portal_status.c" INCLUDE_DIRS "include"
                       REQUIRES esp_common configuration clock PRIV_REQUIRES espressif__cjson heap esp_http_server freertos log
                       EMBED_FILES "web/index.html" "web/portal.css" "web/portal.js")
