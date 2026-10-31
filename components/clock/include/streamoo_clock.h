#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef enum {
    STREAMOO_CLOCK_UTC, STREAMOO_CLOCK_POLAND, STREAMOO_CLOCK_LONDON,
    STREAMOO_CLOCK_BERLIN, STREAMOO_CLOCK_PARIS, STREAMOO_CLOCK_HELSINKI,
    STREAMOO_CLOCK_MADRID, STREAMOO_CLOCK_ROME, STREAMOO_CLOCK_ZONE_COUNT
} streamoo_clock_zone_t;
const char *streamoo_clock_zone_name(streamoo_clock_zone_t);
bool streamoo_clock_zone_parse(const char *, streamoo_clock_zone_t *);
typedef struct {
    uint16_t year;
    uint8_t month,day,hour,minute,second;
} streamoo_clock_civil_t;
/* Pure projection, no libc TZ/global state, network, allocation or I/O.
 * Accepts synchronized Unix UTC2020..2099. Otherwise clears out, returns false.
 * Listed European zones switch on last Sunday March/October at01:00UTC. The hardware
 * clock remains UTC for TLS. Rules are explicit, not an updatable IANA DB. */
bool streamoo_clock_project(int64_t utc_seconds, bool synchronized,
                            streamoo_clock_zone_t, streamoo_clock_civil_t *out);
