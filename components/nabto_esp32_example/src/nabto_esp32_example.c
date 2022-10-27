#include <nabto/nabto_device.h>
#include <nn/log.h>
#include <string.h>

#include "device_event_handler.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nabto_init.h"
#include "nvs_flash.h"
#include "wifi_init.h"

static const char* TAG = "nabto_esp32_example";

#define EXAMPLE_NABTO_PRODUCT_ID CONFIG_NABTO_PRODUCT_ID
#define EXAMPLE_NABTO_DEVICE_ID CONFIG_NABTO_DEVICE_ID

NabtoDevice *nabto_esp32_example_get_device()
{
    return nabto_init_get_device();
}

NabtoDeviceError nabto_esp32_example_set_ids(NabtoDevice *dev)
{
    NabtoDeviceError err =
        nabto_device_set_product_id(dev, EXAMPLE_NABTO_PRODUCT_ID);
    if (err != NABTO_DEVICE_EC_OK) {
        return err;
    }
    ESP_LOGI(TAG, "Setting nabto product id : %s", EXAMPLE_NABTO_PRODUCT_ID);
    err = nabto_device_set_device_id(dev, EXAMPLE_NABTO_DEVICE_ID);
    if (err != NABTO_DEVICE_EC_OK) {
        return err;
    }
    ESP_LOGI(TAG, "Setting nabto device id : %s", EXAMPLE_NABTO_DEVICE_ID);
    return err;
}

bool nabto_esp32_example_load_private_key(NabtoDevice *dev,
                                          nvs_handle_t nvsHandle)
{
    size_t keyLength;
    esp_err_t status = nvs_get_str(nvsHandle, "private_key", NULL, &keyLength);
    if (status == ESP_OK && keyLength < 512) {
        // read key from nvs
        char *key = malloc(512);
        status = nvs_get_str(nvsHandle, "private_key", key, &keyLength);
        if (status == ESP_OK) {
            nabto_device_set_private_key(dev, key);
        } else {
            ESP_LOGI(TAG, "Cannot read private key");
            esp_restart();
            // fail
        }
    } else {
        char *key;
        nabto_device_create_private_key(dev, &key);
        nabto_device_set_private_key(dev, key);
        nvs_set_str(nvsHandle, "private_key", key);
        nabto_device_string_free(key);
    }
    return true;
}
