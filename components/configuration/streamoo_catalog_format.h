#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define STREAMOO_CATALOG_MAGIC "SOOCAT2\n"
#define STREAMOO_CATALOG_MAGIC_BYTES 8u
/* Compacts16hex/TAB/name/TAB/url/TAB/ovol into public3-field CSV in-place. */
bool streamoo_catalog_decode_ids(uint8_t *, size_t *, uint64_t *, size_t,
                                 size_t *, bool);
