#pragma once
#include "streamoo_decoder.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct streamoo_flac_alt streamoo_flac_alt_t;
esp_err_t streamoo_flac_alt_open(streamoo_flac_alt_t **);
esp_err_t streamoo_flac_alt_process(streamoo_flac_alt_t *,uint8_t *,size_t,bool,
                                   uint8_t *,size_t,streamoo_decode_result_t *);
esp_err_t streamoo_flac_alt_reset(streamoo_flac_alt_t *);
void streamoo_flac_alt_close(streamoo_flac_alt_t *);
#ifdef __cplusplus
}
#endif
