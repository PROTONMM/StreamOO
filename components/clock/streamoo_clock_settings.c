#include "streamoo_clock_settings.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
static portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
static SemaphoreHandle_t writes;
static streamoo_clock_zone_t zone=STREAMOO_CLOCK_POLAND;
static bool ready,persisted;
streamoo_clock_zone_t streamoo_clock_settings_get(void){
    portENTER_CRITICAL(&lock);streamoo_clock_zone_t out=zone;portEXIT_CRITICAL(&lock);return out;
}
esp_err_t streamoo_clock_settings_start(void){
    if(writes)return ESP_ERR_INVALID_STATE;
    writes=xSemaphoreCreateMutex();if(!writes)return ESP_ERR_NO_MEM;
    esp_err_t error=nvs_flash_init();if(error!=ESP_OK)return error;
    nvs_handle_t handle;error=nvs_open("streamoo_clock",NVS_READONLY,&handle);
    if(error==ESP_ERR_NVS_NOT_FOUND){ready=true;return ESP_OK;}
    if(error!=ESP_OK)return error;
    char name[32];size_t n=sizeof name;
    error=nvs_get_str(handle,"zone",name,&n);nvs_close(handle);
    if(error!=ESP_OK && error!=ESP_ERR_NVS_NOT_FOUND)return error;
    streamoo_clock_zone_t loaded;
    if(error==ESP_OK && streamoo_clock_zone_parse(name,&loaded)){
        portENTER_CRITICAL(&lock);zone=loaded;portEXIT_CRITICAL(&lock);persisted=true;
    }
    ready=true;return ESP_OK;
}
esp_err_t streamoo_clock_settings_set(const char *name){
    streamoo_clock_zone_t next;
    if(!streamoo_clock_zone_parse(name,&next))return ESP_ERR_INVALID_ARG;
    if(!ready||!writes)return ESP_ERR_INVALID_STATE;
    if(xSemaphoreTake(writes,pdMS_TO_TICKS(5000))!=pdTRUE)return ESP_ERR_TIMEOUT;
    if(persisted && next==streamoo_clock_settings_get()){xSemaphoreGive(writes);return ESP_OK;}
    nvs_handle_t handle;esp_err_t error=nvs_open("streamoo_clock",NVS_READWRITE,&handle);
    if(error==ESP_OK){
        error=nvs_set_str(handle,"zone",name);
        if(error==ESP_OK)error=nvs_commit(handle);
        nvs_close(handle);
    }
    if(error==ESP_OK){
        portENTER_CRITICAL(&lock);zone=next;portEXIT_CRITICAL(&lock);persisted=true;
    }
    xSemaphoreGive(writes);return error;
}
