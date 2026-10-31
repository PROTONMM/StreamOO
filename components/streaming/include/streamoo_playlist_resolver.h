#pragma once
#include <stdbool.h>
#include <stddef.h>
#define STREAMOO_PLAYLIST_LIMIT 16384u
#define STREAMOO_URL_LIMIT 2048u
typedef enum { STREAMOO_PLAYLIST_OK, STREAMOO_PLAYLIST_INVALID, STREAMOO_PLAYLIST_HLS } streamoo_playlist_result_t;
/* Pure bounded resolver. Document needs bytes+1 capacity; mutates document; first M3U URL / lowest FileN in PLS.
 * Relative URLs resolve against final response URL. No file, credentials or TLS downgrade. */
bool streamoo_stream_url_allowed(const char *url);
bool streamoo_playlist_url(const char *url);
streamoo_playlist_result_t streamoo_playlist_resolve(char *document, size_t bytes,
    const char *base_url, char out[STREAMOO_URL_LIMIT+1]);
