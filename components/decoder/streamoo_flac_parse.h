#pragma once
#include "esp_es_parse_types.h"

/* Original incremental parser through the public simple-decoder registration API.
 * Cache ownership and frame decoder remain in the SDK. extra_data uses free(). */
esp_es_parse_err_t streamoo_flac_parse(esp_es_parse_raw_t *,esp_es_parse_frame_info_t *);
