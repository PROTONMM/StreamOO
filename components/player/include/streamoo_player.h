#pragma once
#include "esp_err.h"
#include "streamoo_ui_controls.h"
#include "streamoo_metering.h"
typedef enum {
  STREAMOO_PLAYER_STOPPED,
  STREAMOO_PLAYER_WAIT_NETWORK,
  STREAMOO_PLAYER_CONNECTING,
  STREAMOO_PLAYER_BUFFERING,
  STREAMOO_PLAYER_PLAYING,
  STREAMOO_PLAYER_RETRY
} streamoo_player_phase_t;
typedef struct {
  uint64_t station_id, generation;
  uint8_t volume;
  uint32_t volume_revision, rate, bitrate_kbps;
  bool ready, running, bitrate_measured;
  streamoo_player_phase_t phase;
  esp_err_t error;
  char title[257];
  char codec[8]; /* Empty until decoded PCM format is known. */
  streamoo_metering_state_t metering;
} streamoo_player_state_t;
/* Task-only service; ready means command owner exists, not that I2S is open.
 * Posts copy commands without waiting for network I/O. UI selection includes
 * the immutable catalog generation; stale selection returns false. */
esp_err_t streamoo_player_start(uint8_t volume);
bool streamoo_player_select(void *, const streamoo_ui_select_command_t *);
bool streamoo_player_control(void *, const streamoo_control_command_t *);
bool streamoo_player_state(streamoo_player_state_t *);

/* Task-only visualization preference, independent of playback commands. */
void streamoo_player_spectrum_enabled(bool);

/* Explicit/idempotent web/API commands, copied to the same owner queue. */
bool streamoo_player_set_playing(bool running);
bool streamoo_player_set_volume(uint8_t volume);
