#pragma once
#include "streamoo_ui_controls.h"
/* Pure diagnostic command owner model, not a network player. */
typedef struct { uint8_t volume; bool running; uint32_t volume_revision; } streamoo_audio_control_model_t;
bool streamoo_audio_control_model_init(streamoo_audio_control_model_t *model, uint8_t volume);
bool streamoo_audio_control_model_apply(streamoo_audio_control_model_t *model,
                                        const streamoo_control_command_t *command);
bool streamoo_audio_control_model_gain(const streamoo_audio_control_model_t *model, uint32_t *gain);
