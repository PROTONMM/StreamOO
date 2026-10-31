#include "sdkconfig.h"
#include "esp_log.h"
#include "streamoo_board.h"
#include "streamoo_platform.h"
#include "streamoo_input_probe.h"
#include "display_smoke.h"
#include "audio_probe.h"
#include "audio_control_probe.h"
#include "streamoo_preferences.h"
#include "streamoo_network.h"
#include "network_portal.h"
#include "streamoo_player.h"

static const char *TAG = "StreamOO";

void app_main(void)
{
    if (streamoo_board_validate() != ESP_OK) {
        ESP_LOGE(TAG, "Invalid hardware configuration; peripherals remain disabled");
        return;
    }
    if (streamoo_platform_start() != ESP_OK) {
        ESP_LOGE(TAG, "Platform startup failed; peripherals remain disabled");
        return;
    }
    ESP_LOGI(TAG, "StreamOO scaffold (no radio functionality yet)");
    ESP_LOGI(TAG, "SSD1322 SPI: MOSI=%d SCLK=%d CS=%d DC=%d",
             CONFIG_STREAMOO_LCD_MOSI, CONFIG_STREAMOO_LCD_SCLK,
             CONFIG_STREAMOO_LCD_CS, CONFIG_STREAMOO_LCD_DC);
    ESP_LOGI(TAG, "I2S: DATA=%d BCLK=%d WS=%d",
             CONFIG_STREAMOO_I2S_DOUT, CONFIG_STREAMOO_I2S_BCLK,
             CONFIG_STREAMOO_I2S_LRCLK);
    ESP_LOGI(TAG, "Encoder: A=%d B=%d BTN=%d",
             CONFIG_STREAMOO_ENCODER_GPIO_A, CONFIG_STREAMOO_ENCODER_GPIO_B,
             CONFIG_STREAMOO_ENCODER_GPIO_BUTTON);
#if defined(CONFIG_STREAMOO_ENCODER_UI_TEST)
    const esp_err_t prefs_error = streamoo_preferences_start();
    if (prefs_error != ESP_OK)
        ESP_LOGW(TAG, "Preferences unavailable: %s; using defaults, no erase", esp_err_to_name(prefs_error));
#endif
#if defined(CONFIG_STREAMOO_ENCODER_GESTURE_TEST)
    if (streamoo_input_probe_start() != ESP_OK) {
        ESP_LOGE(TAG, "Input probe could not start");
    }
#endif
#if defined(CONFIG_STREAMOO_AUDIO_CONTROL_TEST) || defined(CONFIG_STREAMOO_I2S_SMOKE_TEST)
    if (streamoo_network_boot_ap_enabled()) {
        ESP_LOGI(TAG,"Fixture audio disabled: network player owns audio");
    } else {
#if defined(CONFIG_STREAMOO_AUDIO_CONTROL_TEST)
    streamoo_preferences_value_t restored = {50,0};
    streamoo_preferences_initial(&restored);
    if (streamoo_audio_control_start(restored.volume) != ESP_OK)
        ESP_LOGE(TAG, "Controlled audio test could not start");
#elif defined(CONFIG_STREAMOO_I2S_SMOKE_TEST)
    if (streamoo_audio_probe_start() != ESP_OK)
        ESP_LOGE(TAG, "Audio probe could not start");
#endif
    }
#endif
#if defined(CONFIG_STREAMOO_NETWORK_AP_TEST)
    if(streamoo_network_boot_ap_enabled()) {
        streamoo_preferences_value_t initial={50,0};streamoo_preferences_initial(&initial);
        if(streamoo_player_start(initial.volume)!=ESP_OK)ESP_LOGE(TAG,"Network player startup failed");
    }
#endif
#if defined(CONFIG_STREAMOO_LCD_SMOKE_TEST)
    if (streamoo_display_smoke_start() != ESP_OK) {
        ESP_LOGE(TAG, "Display smoke task could not start");
    }
#endif
#if defined(CONFIG_STREAMOO_NETWORK_AP_TEST)
    esp_err_t network_error=streamoo_network_start(&streamoo_app_network_hooks);
    if (network_error!=ESP_OK)
        ESP_LOGW(TAG,"AP test inactive: %s; see docs/NETWORK_BRINGUP.md",esp_err_to_name(network_error));
#endif
}
