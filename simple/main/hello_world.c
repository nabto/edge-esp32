#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"

#include "nabto_esp32_example.h"


#include <nabto/nabto_device.h>
#include <nn/log.h>

static const char *TAG = "simple demo";

void app_main(void)
{
    nabto_esp32_example_init();

    NabtoDevice* dev = nabto_esp32_example_get_device();

    NabtoDeviceListener* listener = nabto_device_listener_new(dev);

    const char* path[] = {"hello-world", NULL };

    nabto_device_coap_init_listener(dev, listener, NABTO_DEVICE_COAP_GET, path);

    NabtoDeviceFuture* future = nabto_device_future_new(dev);

    while (true) {
        NabtoDeviceCoapRequest* request;
        nabto_device_listener_new_coap_request(listener, future, &request);
        NabtoDeviceError ec = nabto_device_future_wait(future);

        ESP_LOGI(TAG, "Got CoAP request");

        if (ec) {
            esp_restart();
        }

        nabto_device_coap_response_set_code(request, 205);
        const char* hello = "HelloWorld!";
        nabto_device_coap_response_set_payload(request, hello, strlen(hello));
        nabto_device_coap_response_set_content_format(request, NABTO_DEVICE_COAP_CONTENT_FORMAT_TEXT_PLAIN_UTF8);
        nabto_device_coap_response_ready(request);
        nabto_device_coap_request_free(request);
    }

    nabto_device_future_free(future);

    ESP_LOGI(TAG, "Waiting forever\n");
    for (int i = 0; ; i++) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fflush(stdout);
    }

    esp_restart();
}
