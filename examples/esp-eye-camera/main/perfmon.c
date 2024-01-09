#include "esp32_perfmon.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "sdkconfig.h"

#include "esp_log.h"
static const char *TAG = "perfmon";

static uint64_t idle0Calls = 0;
static uint64_t idle1Calls = 0;

/*
#if defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_240)
static const uint64_t MaxIdleCalls = 1855000;
#elif defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_160)
static const uint64_t MaxIdleCalls = 1233100;
#else
#error "Unsupported CPU frequency"
#endif
*/

static bool idle_task_0()
{
    idle0Calls += 1;
    return false;
}

static bool idle_task_1()
{
    idle1Calls += 1;
    return false;
}

#define PERF_INTERVAL 10

static void perfmon_task(void *args)
{
    while (1)
	{
            ESP_LOGI(TAG, "Core 0 idlecalls %llu per second", idle0Calls / PERF_INTERVAL);
            ESP_LOGI(TAG, "Core 1 idlecalls %llu per second", idle1Calls / PERF_INTERVAL);

            idle0Calls = 0;
            idle1Calls = 0;
            
            ESP_LOGI(TAG, "Heap allocation dump:");
            heap_caps_print_heap_info(MALLOC_CAP_8BIT);
            
            
            // TODO configurable delay
            vTaskDelay(PERF_INTERVAL * 1000 / portTICK_PERIOD_MS);
	}
    vTaskDelete(NULL);
}

esp_err_t perfmon_start()
{
    ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_0, 0));
    ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_1, 1));
    // TODO calculate optimal stack size
    xTaskCreate(perfmon_task, "perfmon", 2048, NULL, 1, NULL);
    return ESP_OK;
}
