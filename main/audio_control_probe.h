#pragma once
#include "esp_err.h"
#include "audio_control_model.h"
/* Diagnostic owner, not the future network player. Single UI producer. */
typedef struct {
    streamoo_audio_control_model_t model;
    bool ready;
    esp_err_t error;
} streamoo_audio_control_state_t;
esp_err_t streamoo_audio_control_start(uint8_t volume);
bool streamoo_audio_control_post(void *context, const streamoo_control_command_t *command);
bool streamoo_audio_control_state(streamoo_audio_control_state_t *out);
