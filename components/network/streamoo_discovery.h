#pragma once
#include <stdbool.h>
#include <stdint.h>
/* Network-owner task only; never called from audio or event callbacks. */
void streamoo_discovery_update(bool connected, const char *ap_name, int64_t now_us);
