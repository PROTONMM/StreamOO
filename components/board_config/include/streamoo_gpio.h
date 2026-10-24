#pragma once

#include <stddef.h>

typedef struct {
    const char *name;
    int gpio;
    int optional;
} streamoo_pin_t;

typedef enum {
    STREAMOO_GPIO_OK = 0,
    STREAMOO_GPIO_INVALID,
    STREAMOO_GPIO_MEMORY,
    STREAMOO_GPIO_STRAPPING,
    STREAMOO_GPIO_CONFLICT,
    STREAMOO_GPIO_USB,
    STREAMOO_GPIO_CONSOLE,
} streamoo_gpio_error_t;

/* Conservative N16R8 policy: no flash/PSRAM or boot-strapping signals.
 * USB and UART-console ownership are supplied by the platform adapter.
 * On failure, index identifies the first rejected signal. No GPIO is touched. */
streamoo_gpio_error_t streamoo_gpio_validate(const streamoo_pin_t *pins,
                                            size_t count, int usb_owned,
                                            int uart_owned, size_t *index);
const char *streamoo_gpio_error_string(streamoo_gpio_error_t error);
