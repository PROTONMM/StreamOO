#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STREAMOO_PREFERENCES_BYTES 8
#define STREAMOO_PREFERENCES_DELAY_MS 5000

typedef struct { uint8_t volume, visualization; } streamoo_preferences_value_t;
typedef struct {
    streamoo_preferences_value_t current, saved;
    uint64_t changed_ms, attempted_ms;
    bool dirty, attempted;
} streamoo_preferences_policy_t;

bool streamoo_preferences_value_valid(streamoo_preferences_value_t value);
bool streamoo_preferences_equal(streamoo_preferences_value_t a, streamoo_preferences_value_t b);
bool streamoo_preferences_encode(streamoo_preferences_value_t value,
                                   uint8_t *out, size_t bytes);
bool streamoo_preferences_decode(const uint8_t *data, size_t bytes,
                                   streamoo_preferences_value_t *out);
bool streamoo_preferences_policy_init(streamoo_preferences_policy_t *policy,
                                       streamoo_preferences_value_t saved);
/* Consume a CHANGE record, coalescing may hide intermediate values. Reset
 * deadline even if value equals current. Reverting to saved cancels dirty.
 * Producer must filter unchanged snapshots. Single owner; monotonic ms. */
bool streamoo_preferences_observe(streamoo_preferences_policy_t *policy,
                                   streamoo_preferences_value_t value, uint64_t now);
bool streamoo_preferences_due(const streamoo_preferences_policy_t *policy, uint64_t now);
/* Call after attempting to persist current. Failure retains dirty state and
 * rate-limits retries to 5s; success is acknowledged only after commit. */
void streamoo_preferences_result(streamoo_preferences_policy_t *policy, bool success, uint64_t now);
