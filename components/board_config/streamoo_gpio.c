#include "streamoo_gpio.h"

streamoo_gpio_error_t streamoo_gpio_validate(const streamoo_pin_t *pins,
                                            size_t count, int usb_owned,
                                            int uart_owned, size_t *index)
{
    for (size_t i = 0; i < count; ++i) {
        const int p = pins[i].gpio;
        streamoo_gpio_error_t error = STREAMOO_GPIO_OK;
        if (p == -1 && pins[i].optional) {
            continue;
        }
        if (p < 0 || p > 48 || (p >= 22 && p <= 25)) {
            error = STREAMOO_GPIO_INVALID;
        } else if (p >= 26 && p <= 37) {
            error = STREAMOO_GPIO_MEMORY;
        } else if (p == 0 || p == 3 || p == 45 || p == 46) {
            error = STREAMOO_GPIO_STRAPPING;
        } else if (usb_owned && (p == 19 || p == 20)) {
            error = STREAMOO_GPIO_USB;
        } else if (uart_owned && (p == 43 || p == 44)) {
            error = STREAMOO_GPIO_CONSOLE;
        } else {
            for (size_t j = 0; j < i; ++j) {
                if (pins[j].gpio == p) {
                    error = STREAMOO_GPIO_CONFLICT;
                    break;
                }
            }
        }
        if (error != STREAMOO_GPIO_OK) {
            if (index) {
                *index = i;
            }
            return error;
        }
    }
    return STREAMOO_GPIO_OK;
}

const char *streamoo_gpio_error_string(streamoo_gpio_error_t error)
{
    switch (error) {
    case STREAMOO_GPIO_OK: return "valid";
    case STREAMOO_GPIO_INVALID: return "invalid GPIO on ESP32-S3";
    case STREAMOO_GPIO_MEMORY: return "reserved for N16R8 flash/PSRAM";
    case STREAMOO_GPIO_STRAPPING: return "boot-strapping GPIO forbidden by board policy";
    case STREAMOO_GPIO_CONFLICT: return "GPIO conflict";
    case STREAMOO_GPIO_USB: return "GPIO owned by USB";
    case STREAMOO_GPIO_CONSOLE: return "GPIO owned by UART console";
    default: return "unknown validation error";
    }
}
