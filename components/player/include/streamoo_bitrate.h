#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Decoder-owner estimate: compressed bytes per decoded audio duration, never
 * network wall time. No allocation/I/O. Prime after the first PCM packet to
 * exclude initial metadata; publish after at least two seconds of source PCM. */
typedef struct {
    uint64_t consumed, produced;
    uint32_t rate, kbps;
    uint8_t channels, bits;
    bool primed;
    uint8_t changes;
    bool variable;
} streamoo_bitrate_t;

static inline uint32_t streamoo_bitrate_update(streamoo_bitrate_t *s,
    uint64_t consumed, uint64_t produced, uint32_t rate, uint8_t channels, uint8_t bits)
{
    if (!s) return 0;
    if (rate < 8000 || rate > 48000 || channels < 1 || channels > 2 ||
        (bits != 16 && bits != 24 && bits != 32)) {
        *s = (streamoo_bitrate_t){0};
        return 0;
    }
    if (!s->primed || s->rate != rate || s->channels != channels || s->bits != bits ||
        consumed < s->consumed || produced < s->produced) {
        *s = (streamoo_bitrate_t){.consumed=consumed, .produced=produced,
            .rate=rate, .channels=channels, .bits=bits, .primed=produced!=0};
        return 0;
    }
    const uint64_t frame_bytes = (uint64_t)channels * (bits / 8);
    const uint64_t pcm = produced - s->produced;
    if (pcm < (uint64_t)rate * frame_bytes * 2) return s->kbps;
    const uint64_t compressed = consumed - s->consumed;
    const uint64_t factor = 8 * (uint64_t)rate * frame_bytes;
    uint64_t value = 0;
    if (compressed <= UINT64_MAX / factor && pcm <= UINT64_MAX / 1000) {
        const uint64_t denominator = pcm * 1000;
        const uint64_t numerator = compressed * factor;
        value = numerator / denominator;
        if (numerator % denominator >= (denominator + 1) / 2) ++value;
    }
    uint32_t next = value > 0 && value <= 4096 ? (uint32_t)value : 0;
    /* Observe decoded-duration windows, not HTTP throughput or UI polls.
     * Three substantial changes filter rounding/padding and a single outlier.
     * This is evidence of variation, not a bitstream CBR/VBR declaration. */
    if (s->kbps && next) {
        uint32_t high = next > s->kbps ? next : s->kbps;
        uint32_t delta = next > s->kbps ? next - s->kbps : s->kbps - next;
        if (delta > 8 && delta > high / 20 && s->changes < 3) ++s->changes;
        if (s->changes >= 3) s->variable = true;
    }
    s->kbps = next;
    s->consumed=consumed; s->produced=produced;
    return s->kbps;
}
