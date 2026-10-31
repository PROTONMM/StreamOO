#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_WIFI_MAX 8u
#define STREAMOO_PLAYLIST_MAX_BYTES (256u * 1024u)
#define STREAMOO_PLAYLIST_MAX_ROWS 2048u
typedef struct {
  char ssid[33], password[64];
} streamoo_wifi_profile_t;
typedef struct {
  uint8_t count;
  streamoo_wifi_profile_t profiles[STREAMOO_WIFI_MAX];
} streamoo_wifi_profiles_t;
bool streamoo_wifi_profiles_valid(const streamoo_wifi_profiles_t *);
bool streamoo_wifi_profiles_parse(const char *, size_t,
                                  const streamoo_wifi_profiles_t *,
                                  streamoo_wifi_profiles_t *);
size_t streamoo_wifi_profiles_encode(const streamoo_wifi_profiles_t *,
                                     uint8_t *, size_t);
bool streamoo_wifi_profiles_decode(const uint8_t *, size_t,
                                   streamoo_wifi_profiles_t *);
typedef struct {
  uint64_t hash;
  uint32_t offset;
  uint16_t bytes;
} streamoo_playlist_seen_t;
typedef struct {
  streamoo_playlist_seen_t seen[STREAMOO_PLAYLIST_MAX_ROWS];
} streamoo_playlist_workspace_t;
typedef struct {
  uint32_t rows, normalized, duplicates, error_line;
  const char *error;
} streamoo_playlist_report_t;
/* Mutable REQUEST COPY only. No I/O/alloc. Canonicalizes BOM/CRLF and leading
 * URL U+200B; caller presents report before commit. Retains duplicate rows. */
bool streamoo_playlist_validate(uint8_t *, size_t *,
                                streamoo_playlist_report_t *,
                                streamoo_playlist_workspace_t *);
