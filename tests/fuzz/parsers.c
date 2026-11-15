/* Original host-only harness: no vendor decoder or hardware execution. */
#include "streamoo_hls.h"
#include "streamoo_icy.h"
#include "streamoo_config_model.h"
#include "streamoo_playlist_resolver.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef STREAMOO_FUZZ_TARGET
#error Select a target: 1 playlist, 2 HLS, 3 ICY, 4 station TSV, 5 Wi-Fi JSON
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > STREAMOO_PLAYLIST_MAX_BYTES + 1u)
    return 0;
  char *copy = malloc(size + 1);
  if (!copy)
    abort();
  memcpy(copy, data, size);
  copy[size] = 0;
#if STREAMOO_FUZZ_TARGET == 1
  char out[STREAMOO_URL_LIMIT + 1];
  streamoo_playlist_resolve(copy, size, "https://radio.invalid/a/list.m3u", out);
#elif STREAMOO_FUZZ_TARGET == 2
  streamoo_hls_manifest_t *manifest = malloc(sizeof *manifest);
  if (!manifest)
    abort();
  streamoo_hls_detect(data, size);
  streamoo_hls_parse(copy, size, "https://radio.invalid/a/list.m3u8", manifest);
  free(manifest);
#elif STREAMOO_FUZZ_TARGET == 3
  streamoo_icy_t icy;
  uint8_t out[1024];
  streamoo_icy_init(&icy, size ? data[0] : 0);
  for (size_t at = 0; at < size;) {
    size_t n = size - at, written;
    if (n > sizeof out)
      n = sizeof out;
    if (!streamoo_icy_feed(&icy, data + at, n, out, sizeof out, &written) || written > n)
      abort();
    at += n;
  }
  size_t written;
  if (!streamoo_icy_feed(&icy, NULL, 0, out, sizeof out, &written) || written)
    abort();
#elif STREAMOO_FUZZ_TARGET == 4
  streamoo_playlist_workspace_t *workspace = calloc(1, sizeof *workspace);
  streamoo_playlist_report_t report;
  if (!workspace)
    abort();
  streamoo_playlist_validate((uint8_t *)copy, &size, &report, workspace);
  free(workspace);
#elif STREAMOO_FUZZ_TARGET == 5
  streamoo_wifi_profiles_t profiles;
  streamoo_wifi_profiles_parse(copy, size, NULL, &profiles);
#else
#error Unknown target
#endif
  free(copy);
  return 0;
}

#ifdef STREAMOO_FUZZ_STANDALONE
/* AFL++ uses stdin or @@; also permits replay without a fuzzing runtime. */
int main(int argc, char **argv) {
  if (argc > 2)
    return 2;
  FILE *input = argc == 2 ? fopen(argv[1], "rb") : stdin;
  if (!input)
    return 2;
  size_t capacity = STREAMOO_PLAYLIST_MAX_BYTES + 2u;
  uint8_t *data = malloc(capacity);
  if (!data)
    abort();
  size_t size = fread(data, 1, capacity, input);
  int error = ferror(input);
  if (input != stdin)
    fclose(input);
  if (!error)
    LLVMFuzzerTestOneInput(data, size);
  free(data);
  return error ? 2 : 0;
}
#endif
