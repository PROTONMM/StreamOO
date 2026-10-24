#include "streamoo_preferences.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static QueueHandle_t updates;
static nvs_handle_t handle;
static streamoo_preferences_value_t initial, last_submitted;
typedef struct { streamoo_preferences_value_t value; uint64_t at_ms; } update_t;

static uint64_t now_ms(void) { return (uint64_t)esp_timer_get_time()/1000; }
static void storage_task(void *context)
{
    (void)context;
    streamoo_preferences_policy_t policy;
    streamoo_preferences_policy_init(&policy,initial);
    for (;;) {
        update_t update;
        if (xQueueReceive(updates,&update,pdMS_TO_TICKS(100) ? pdMS_TO_TICKS(100) : 1)==pdTRUE)
            streamoo_preferences_observe(&policy,update.value,update.at_ms);
        uint64_t now=now_ms();
        if (!streamoo_preferences_due(&policy,now)) continue;
        uint8_t blob[STREAMOO_PREFERENCES_BYTES];
        streamoo_preferences_encode(policy.current,blob,sizeof blob);
        esp_err_t err=nvs_set_blob(handle,"ui_prefs",blob,sizeof blob);
        if (err==ESP_OK) err=nvs_commit(handle);
        streamoo_preferences_result(&policy,err==ESP_OK,now_ms());
        if (err==ESP_OK)
            ESP_LOGI("preferences","Saved volume=%u widget=%u",policy.saved.volume,policy.saved.visualization+1);
        else ESP_LOGW("preferences","Save failed: %s; retry after 5s",esp_err_to_name(err));
    }
}
esp_err_t streamoo_preferences_start(void)
{
    if (updates) return ESP_ERR_INVALID_STATE;
    esp_err_t err=nvs_flash_init();
    if (err!=ESP_OK) return err; /* Deliberately no erase/reformat fallback. */
    err=nvs_open("streamoo",NVS_READWRITE,&handle);
    if (err!=ESP_OK) return err;
    initial=(streamoo_preferences_value_t){50,0};
    uint8_t blob[STREAMOO_PREFERENCES_BYTES];
    size_t bytes=sizeof blob;
    err=nvs_get_blob(handle,"ui_prefs",blob,&bytes);
    if (err==ESP_OK && !streamoo_preferences_decode(blob,bytes,&initial)) err=ESP_ERR_INVALID_RESPONSE;
    if (err!=ESP_OK && err!=ESP_ERR_NVS_NOT_FOUND) { nvs_close(handle); return err; }
    last_submitted=initial;
    updates=xQueueCreate(1,sizeof(update_t));
    if (!updates) { nvs_close(handle); return ESP_ERR_NO_MEM; }
    if (xTaskCreate(storage_task,"preferences",3072,NULL,1,NULL)!=pdPASS) {
        vQueueDelete(updates); updates=NULL; nvs_close(handle); return ESP_ERR_NO_MEM;
    }
    ESP_LOGI("preferences","Loaded volume=%u widget=%u; writes after 5s idle",initial.volume,initial.visualization+1);
    return ESP_OK;
}
bool streamoo_preferences_initial(streamoo_preferences_value_t *out)
{
    if (!updates || !out) return false;
    *out=initial;
    return true;
}
bool streamoo_preferences_submit(streamoo_preferences_value_t value)
{
    if (!updates || !streamoo_preferences_value_valid(value)) return false;
    if (streamoo_preferences_equal(last_submitted,value)) return true;
    update_t update={value,now_ms()};
    if (xQueueOverwrite(updates,&update)!=pdTRUE) return false;
    last_submitted=value;
    return true;
}
