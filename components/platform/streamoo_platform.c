#include "streamoo_platform.h"

#include <inttypes.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
#include "esp_timer.h"
#endif

#if !defined(CONFIG_SPIRAM_MODE_OCT) || !defined(CONFIG_SPIRAM_SPEED_80M) || !defined(CONFIG_ESPTOOLPY_FLASHSIZE_16MB)
#error "StreamOO N16R8 requires 16 MB flash and Octal PSRAM at 80 MHz"
#endif
#if CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ != 240
#error "StreamOO N16R8 profile requires CPU 240 MHz"
#endif

static const char *TAG = "platform";

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
#define STREAMOO_CPU_PROFILE_MAX_TASKS 48u
#define STREAMOO_CPU_PROFILE_TOP_TASKS 6u

typedef struct {
    TaskHandle_t handle;
    uint64_t runtime_us;
} streamoo_cpu_sample_t;

static TaskStatus_t cpu_tasks[STREAMOO_CPU_PROFILE_MAX_TASKS];
static streamoo_cpu_sample_t cpu_previous[STREAMOO_CPU_PROFILE_MAX_TASKS];
static UBaseType_t cpu_previous_count;
static int64_t cpu_previous_at_us;

static uint64_t cpu_previous_runtime(TaskHandle_t handle, bool *found)
{
    for (UBaseType_t i = 0; i < cpu_previous_count; ++i) {
        if (cpu_previous[i].handle == handle) {
            *found = true;
            return cpu_previous[i].runtime_us;
        }
    }
    *found = false;
    return 0;
}

static UBaseType_t cpu_affinity_mask(TaskHandle_t handle)
{
#if defined(CONFIG_FREERTOS_SMP) && CONFIG_FREERTOS_SMP
    return vTaskCoreAffinityGet(handle);
#else
    const BaseType_t core = xTaskGetCoreID(handle);
    return core == tskNO_AFFINITY ? 0x3u : (1u << (unsigned)core);
#endif
}

static unsigned cpu_percent_tenths(uint64_t runtime_us, uint64_t interval_us)
{
    if (interval_us == 0) {
        return 0;
    }
    return (unsigned)((runtime_us * 1000u + interval_us / 2u) / interval_us);
}

static void log_cpu_task(const TaskStatus_t *task, uint64_t runtime_us,
                         uint64_t interval_us)
{
    const unsigned percent = cpu_percent_tenths(runtime_us, interval_us);
    ESP_LOGI(TAG, "CPU task=%s load=%u.%u%% affinity=0x%" PRIx32
             " stack_free=%uB",
             task->pcTaskName, percent / 10u, percent % 10u,
             (uint32_t)cpu_affinity_mask(task->xHandle),
             (unsigned)task->usStackHighWaterMark);
}

static void log_cpu(void)
{
    const UBaseType_t expected = uxTaskGetNumberOfTasks();
    if (expected > STREAMOO_CPU_PROFILE_MAX_TASKS) {
        ESP_LOGW(TAG, "CPU profile skipped: tasks=%u exceeds fixed capacity=%u",
                 (unsigned)expected, STREAMOO_CPU_PROFILE_MAX_TASKS);
        return;
    }
    uint64_t total_runtime = 0;
    const UBaseType_t count = uxTaskGetSystemState(cpu_tasks,
                                                    STREAMOO_CPU_PROFILE_MAX_TASKS,
                                                    &total_runtime);
    const int64_t now = esp_timer_get_time();
    if (count == 0 || count > STREAMOO_CPU_PROFILE_MAX_TASKS) {
        ESP_LOGW(TAG, "CPU profile snapshot unavailable (tasks=%u)", (unsigned)count);
        return;
    }
    if (cpu_previous_at_us == 0) {
        for (UBaseType_t i = 0; i < count; ++i) {
            cpu_previous[i].handle = cpu_tasks[i].xHandle;
            cpu_previous[i].runtime_us = cpu_tasks[i].ulRunTimeCounter;
        }
        cpu_previous_count = count;
        cpu_previous_at_us = now;
        ESP_LOGI(TAG, "CPU profile warm-up: next health interval reports per-task load");
        return;
    }
    const uint64_t interval_us = (uint64_t)(now - cpu_previous_at_us);
    uint64_t idle_runtime[2] = {0, 0};
    uint64_t runtime[STREAMOO_CPU_PROFILE_MAX_TASKS] = {0};
    for (UBaseType_t i = 0; i < count; ++i) {
        bool found = false;
        const uint64_t previous = cpu_previous_runtime(cpu_tasks[i].xHandle, &found);
        runtime[i] = found ? (uint64_t)cpu_tasks[i].ulRunTimeCounter - previous : 0;
        if (strcmp(cpu_tasks[i].pcTaskName, "IDLE0") == 0) {
            idle_runtime[0] = runtime[i];
        } else if (strcmp(cpu_tasks[i].pcTaskName, "IDLE1") == 0) {
            idle_runtime[1] = runtime[i];
        }
    }
    const unsigned idle0 = cpu_percent_tenths(idle_runtime[0], interval_us);
    const unsigned idle1 = cpu_percent_tenths(idle_runtime[1], interval_us);
    ESP_LOGI(TAG, "CPU interval=%" PRIu64 "ms core0_busy=%u.%u%% core1_busy=%u.%u%%"
             " (per-task load is %% of one core)",
             interval_us / 1000u,
             (1000u - (idle0 > 1000u ? 1000u : idle0)) / 10u,
             (1000u - (idle0 > 1000u ? 1000u : idle0)) % 10u,
             (1000u - (idle1 > 1000u ? 1000u : idle1)) / 10u,
             (1000u - (idle1 > 1000u ? 1000u : idle1)) % 10u);
    bool selected[STREAMOO_CPU_PROFILE_MAX_TASKS] = {0};
    for (unsigned rank = 0; rank < STREAMOO_CPU_PROFILE_TOP_TASKS; ++rank) {
        UBaseType_t best = count;
        for (UBaseType_t i = 0; i < count; ++i) {
            if (!selected[i] && strcmp(cpu_tasks[i].pcTaskName, "IDLE0") != 0 &&
                strcmp(cpu_tasks[i].pcTaskName, "IDLE1") != 0 &&
                (best == count || runtime[i] > runtime[best])) {
                best = i;
            }
        }
        if (best == count || runtime[best] == 0) {
            break;
        }
        selected[best] = true;
        log_cpu_task(&cpu_tasks[best], runtime[best], interval_us);
    }
    for (UBaseType_t i = 0; i < count; ++i) {
        cpu_previous[i].handle = cpu_tasks[i].xHandle;
        cpu_previous[i].runtime_us = cpu_tasks[i].ulRunTimeCounter;
    }
    cpu_previous_count = count;
    cpu_previous_at_us = now;
}
#endif

static void log_heap(void)
{
    ESP_LOGI(TAG, "heap internal free=%zu min=%zu largest=%zu; PSRAM free=%zu min=%zu largest=%zu",
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

static void health_task(void *context)
{
    (void)context;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        log_heap();
#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
        log_cpu();
#endif
        ESP_LOGI(TAG, "health stack high-water=%u bytes",
                 (unsigned)uxTaskGetStackHighWaterMark(NULL));
    }
}

esp_err_t streamoo_platform_start(void)
{
    static bool started;
    if (started) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    uint32_t flash_bytes = 0;
    esp_err_t error = esp_flash_get_size(NULL, &flash_bytes);
    if (error != ESP_OK) {
        return error;
    }
    const size_t psram_bytes = esp_psram_is_initialized() ? esp_psram_get_size() : 0;
    ESP_LOGI(TAG, "IDF %s; cores=%u; CPU profile=%d MHz; flash=%" PRIu32 "; PSRAM=%zu",
             esp_get_idf_version(), chip.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
             flash_bytes, psram_bytes);
    if (chip.model != CHIP_ESP32S3 || chip.cores != 2 ||
        flash_bytes != 16 * 1024 * 1024 || psram_bytes != 8 * 1024 * 1024) {
        ESP_LOGE(TAG, "Hardware does not match the N16R8 profile");
        return ESP_ERR_INVALID_STATE;
    }
    log_heap();
    if (xTaskCreate(health_task, "streamoo_health", 3072, NULL, 1, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    started = true;
    return ESP_OK;
}
