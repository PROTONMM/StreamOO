#include "streamoo_board.h"
#include "streamoo_gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

esp_err_t streamoo_board_validate(void)
{
    const streamoo_pin_t pins[] = {
        {"LCD_MOSI", CONFIG_STREAMOO_LCD_MOSI, 0},
        {"LCD_SCLK", CONFIG_STREAMOO_LCD_SCLK, 0},
        {"LCD_CS", CONFIG_STREAMOO_LCD_CS, 0},
        {"LCD_DC", CONFIG_STREAMOO_LCD_DC, 0},
        {"LCD_RST", CONFIG_STREAMOO_LCD_RST, 1},
        {"I2S_DOUT", CONFIG_STREAMOO_I2S_DOUT, 0},
        {"I2S_BCLK", CONFIG_STREAMOO_I2S_BCLK, 0},
        {"I2S_LRCLK", CONFIG_STREAMOO_I2S_LRCLK, 0},
        {"ENCODER_GPIO_A", CONFIG_STREAMOO_ENCODER_GPIO_A, 0},
        {"ENCODER_GPIO_B", CONFIG_STREAMOO_ENCODER_GPIO_B, 0},
        {"ENCODER_GPIO_BUTTON", CONFIG_STREAMOO_ENCODER_GPIO_BUTTON, 0},
#if defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
        {"ESP_CONSOLE_UART_TX_GPIO", CONFIG_ESP_CONSOLE_UART_TX_GPIO, 0},
        {"ESP_CONSOLE_UART_RX_GPIO", CONFIG_ESP_CONSOLE_UART_RX_GPIO, 1},
#endif
    };
    size_t index = 0;
    int usb_owned = 0;
    int uart_owned = 0;
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG) || defined(CONFIG_ESP_CONSOLE_USB_CDC) || defined(CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG) || defined(CONFIG_STREAMOO_RESERVE_USB)
    usb_owned = 1;
#endif
#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT)
    uart_owned = 1;
#endif
    const streamoo_gpio_error_t error = streamoo_gpio_validate(
        pins, sizeof(pins) / sizeof(pins[0]), usb_owned, uart_owned, &index);
    if (error != STREAMOO_GPIO_OK) {
        ESP_LOGE("board", "%s GPIO%d: %s", pins[index].name,
                 pins[index].gpio, streamoo_gpio_error_string(error));
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI("board", "N16R8 GPIO map validated");
    return ESP_OK;
}
