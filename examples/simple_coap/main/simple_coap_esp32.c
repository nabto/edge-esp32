#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "nabto_esp32_example.h"
#include "hello_world_coap_handler.h"
#include "device_event_handler.h"
#include "connection_event_handler.h"


#include <nabto/nabto_device.h>
#include <nn/log.h>

static const char *TAG = "simple demo";

#define CHECK_NABTO_ERR(err) do { if (err != NABTO_DEVICE_EC_OK) { ESP_LOGE(TAG, "Unexpected error at %s:%d, %s", __FILE__, __LINE__, nabto_device_error_get_message(err) ); esp_restart(); } } while (0)
#define CHECK_NULL(ptr) do { if (ptr == NULL) { ESP_LOGE(TAG, "Unexpected out of memory at %s:%d", __FILE__, __LINE__); esp_restart(); } } while (0)

void app_main(void)
{
    nabto_esp32_example_init_wifi();

    NabtoDevice* dev = nabto_device_new();
    CHECK_NULL(dev);

    CHECK_NABTO_ERR(nabto_esp32_example_set_ids(dev));

    CHECK_NABTO_ERR(nabto_device_enable_mdns(dev));

    struct hello_world_coap_handler coapHandler;
    hello_world_coap_handler_init(&coapHandler, dev);

    struct device_event_handler eventHandler;
    device_event_handler_init(&eventHandler, dev);

    struct connection_event_handler connectionEventHandler;
    connection_event_handler_init(&connectionEventHandler, dev);

    NabtoDeviceFuture* future = nabto_device_future_new(dev);
    CHECK_NULL(future);

    nabto_device_start(dev, future);
    CHECK_NABTO_ERR(nabto_device_future_wait(future));
    nabto_device_future_free(future);

    char* fingerprint = NULL;
    CHECK_NABTO_ERR(nabto_device_get_device_fingerprint(dev, &fingerprint));

    ESP_LOGI(TAG, "Started nabto device with fingerprint %s", fingerprint);
    nabto_device_string_free(fingerprint);


    ESP_LOGI(TAG, "Waiting forever\n");
    for (int i = 0; ; i++) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fflush(stdout);
    }

    esp_restart();
}
