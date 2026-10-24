#pragma once

#include "esp_err.h"
#include "ssd1322_protocol.h"

typedef struct streamoo_ssd1322 streamoo_ssd1322_t;
typedef struct {
    int spi_host; /* 2 or 3, owns the entire previously unused bus. */
    int mosi, sclk, cs, dc, reset; /* reset=-1: reset supplied by board. */
    int clock_hz; /* 1..10 MHz, controller tcycle >=100ns. */
    ssd1322_profile_t panel;
} streamoo_ssd1322_config_t;

/* Single-task ownership for the handle and frame. All calls are synchronous;
 * invoke only from a display/UI task, never from PCM output. Allocations occur
 * only in open. present accepts RAM/PSRAM, stages to internal DMA memory. */
esp_err_t streamoo_ssd1322_open(const streamoo_ssd1322_config_t *config,
                                streamoo_ssd1322_t **out);
esp_err_t streamoo_ssd1322_present(streamoo_ssd1322_t *display,
                                   const uint8_t *frame, size_t length);
esp_err_t streamoo_ssd1322_enable(streamoo_ssd1322_t *display, bool enabled);
esp_err_t streamoo_ssd1322_close(streamoo_ssd1322_t *display);
