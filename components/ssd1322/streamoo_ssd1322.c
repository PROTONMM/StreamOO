#include "streamoo_ssd1322.h"

#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct streamoo_ssd1322 {
    ssd1322_protocol_t protocol;
    spi_device_handle_t spi;
    spi_host_device_t host;
    int dc;
    uint8_t *dma;
};

static int write_spi(void *context, bool data, const uint8_t *bytes, size_t length)
{
    streamoo_ssd1322_t *d = context;
    if (!bytes || !length || length > SSD1322_FRAME_BYTES) return ESP_ERR_INVALID_ARG;
    esp_err_t error = gpio_set_level(d->dc, data);
    if (error != ESP_OK) return error;
    spi_transaction_t transaction = {.length = length * 8};
    if (length <= sizeof(transaction.tx_data)) {
        transaction.flags = SPI_TRANS_USE_TXDATA;
        memcpy(transaction.tx_data, bytes, length);
    } else {
        memcpy(d->dma, bytes, length);
        transaction.tx_buffer = d->dma;
    }
    return spi_device_transmit(d->spi, &transaction);
}

esp_err_t streamoo_ssd1322_open(const streamoo_ssd1322_config_t *c,
                                streamoo_ssd1322_t **out)
{
    if (!out) return ESP_ERR_INVALID_ARG;
    *out = NULL;
    if (!c || c->spi_host < 2 || c->spi_host > 3 || c->clock_hz < 1000000 ||
        c->clock_hz > 10000000 || c->panel.column_offset > 56 ||
        c->panel.master_current > 15 || (c->panel.remap & ~0x16)) {
        return ESP_ERR_INVALID_ARG;
    }
    const int pins[] = {c->mosi, c->sclk, c->cs, c->dc, c->reset};
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        if (i == 4 && pins[i] == -1) continue;
        if (!GPIO_IS_VALID_OUTPUT_GPIO(pins[i])) return ESP_ERR_INVALID_ARG;
        for (size_t j = 0; j < i; ++j) {
            if (pins[j] == pins[i]) return ESP_ERR_INVALID_ARG;
        }
    }
    streamoo_ssd1322_t *d = calloc(1, sizeof(*d));
    if (!d) return ESP_ERR_NO_MEM;
    d->dma = heap_caps_malloc(SSD1322_FRAME_BYTES, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!d->dma) { free(d); return ESP_ERR_NO_MEM; }
    d->dc = c->dc;
    d->host = c->spi_host == 2 ? SPI2_HOST : SPI3_HOST;
    const spi_bus_config_t bus = {
        .mosi_io_num = c->mosi, .miso_io_num = -1, .sclk_io_num = c->sclk,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = SSD1322_FRAME_BYTES,
    };
    esp_err_t error = spi_bus_initialize(d->host, &bus, SPI_DMA_CH_AUTO);
    if (error != ESP_OK) goto free_memory;
    const gpio_config_t gpio = {
        .pin_bit_mask = (1ULL << c->dc) | (c->reset < 0 ? 0 : (1ULL << c->reset)),
        .mode = GPIO_MODE_OUTPUT,
    };
    error = gpio_config(&gpio);
    if (error != ESP_OK) goto free_bus;
    const spi_device_interface_config_t device = {
        .clock_speed_hz = c->clock_hz, .mode = 0,
        .spics_io_num = c->cs, .queue_size = 1,
        .cs_ena_pretrans = 1, .cs_ena_posttrans = 1,
    };
    error = spi_bus_add_device(d->host, &device, &d->spi);
    if (error != ESP_OK) goto free_bus;
    /* Wait at least 300 ms after power stabilization. Tick rounding adds one
     * tick so low tick rates cannot shorten the datasheet minimum. */
    vTaskDelay(pdMS_TO_TICKS(300) + 1);
    if (c->reset >= 0) {
        error = gpio_set_level(c->reset, 0);
        if (error != ESP_OK) goto remove_device;
        vTaskDelay(pdMS_TO_TICKS(1) + 1); /* >=100 us reset pulse */
        error = gpio_set_level(c->reset, 1);
        if (error != ESP_OK) goto remove_device;
        vTaskDelay(pdMS_TO_TICKS(1) + 1);
    }
    error = ssd1322_protocol_init(&d->protocol, write_spi, d, &c->panel);
    if (error != ESP_OK) goto remove_device;
    *out = d;
    return ESP_OK;
remove_device:
    spi_bus_remove_device(d->spi);
free_bus:
    spi_bus_free(d->host);
free_memory:
    heap_caps_free(d->dma);
    free(d);
    return error;
}

esp_err_t streamoo_ssd1322_present(streamoo_ssd1322_t *d,
                                   const uint8_t *frame, size_t length)
{
    if (!d || !frame || length != SSD1322_FRAME_BYTES) return ESP_ERR_INVALID_ARG;
    return ssd1322_protocol_present(&d->protocol, frame, length);
}

esp_err_t streamoo_ssd1322_enable(streamoo_ssd1322_t *d, bool enabled)
{
    if (!d) return ESP_ERR_INVALID_ARG;
    const esp_err_t error = ssd1322_protocol_enable(&d->protocol, enabled);
    if (error == ESP_OK && enabled) vTaskDelay(pdMS_TO_TICKS(200) + 1);
    return error;
}

esp_err_t streamoo_ssd1322_close(streamoo_ssd1322_t *d)
{
    if (!d) return ESP_ERR_INVALID_ARG;
    const esp_err_t off_error = streamoo_ssd1322_enable(d, false);
    esp_err_t error = spi_bus_remove_device(d->spi);
    if (error != ESP_OK) return error;
    error = spi_bus_free(d->host);
    heap_caps_free(d->dma);
    free(d);
    return off_error != ESP_OK ? off_error : error;
}
