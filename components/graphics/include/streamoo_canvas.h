#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Packed row-major 4bpp, high nibble first. Caller owns pixels and lifetime.
 * No allocation, RTOS, display driver, global state or floating point.
 * All drawing respects clip and accepts gray levels 0..15 only.
 * Coordinates/dimensions are signed 16-bit; non-positive rectangles are empty. */
typedef struct { int16_t x, y, width, height; } streamoo_rect_t;
typedef struct {
    uint8_t *pixels;
    size_t bytes;
    int16_t width, height;
    uint16_t stride;
    streamoo_rect_t clip;
    streamoo_rect_t bounds;
} streamoo_canvas_t;

bool streamoo_canvas_init(streamoo_canvas_t *canvas, uint8_t *pixels,
                           size_t bytes, int16_t width, int16_t height, uint16_t stride);
/* Replace clip with the intersection of bounds and this canvas' fixed limits.
 * Use a child canvas to nest clips. Its coordinates stay in surface space. */
void streamoo_canvas_clip(streamoo_canvas_t *canvas, streamoo_rect_t bounds);
bool streamoo_canvas_child(const streamoo_canvas_t *parent, streamoo_rect_t bounds,
                            streamoo_canvas_t *child);
void streamoo_canvas_pixel(streamoo_canvas_t *canvas, int16_t x, int16_t y, uint8_t gray);
void streamoo_canvas_fill(streamoo_canvas_t *canvas, streamoo_rect_t rect, uint8_t gray);
void streamoo_canvas_outline(streamoo_canvas_t *canvas, streamoo_rect_t rect, uint8_t gray);
/* Integer Bresenham. Work bounded by 65536 steps even for extreme endpoints. */
void streamoo_canvas_line(streamoo_canvas_t *canvas, int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1, uint8_t gray);
/* MSB-first row-major 1bpp mask. Set bits paint gray; zero bits are transparent.
 * Source must not overlap destination. Rejected input performs no writes. */
bool streamoo_canvas_mask(streamoo_canvas_t *canvas, int16_t x, int16_t y,
                           int16_t width, int16_t height, const uint8_t *mask,
                           size_t bytes, uint16_t stride, uint8_t gray);
