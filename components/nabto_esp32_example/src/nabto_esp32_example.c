#include <nabto/nabto_device.h>
#include <nabto/nabto_device_experimental.h>
#include <nn/log.h>
#include <string.h>

#include <platform/np_util.h>

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
#include "nvs_flash.h"

static const char* TAG = "nabto_esp32_example";

#define EXAMPLE_NABTO_PRODUCT_ID CONFIG_EXAMPLE_NABTO_PRODUCT_ID
#define EXAMPLE_NABTO_DEVICE_ID CONFIG_EXAMPLE_NABTO_DEVICE_ID
#define EXAMPLE_NABTO_PRIVATE_KEY CONFIG_EXAMPLE_NABTO_PRIVATE_KEY

NabtoDeviceError nabto_esp32_example_set_id_and_key(NabtoDevice *dev)
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

    if (strlen(EXAMPLE_NABTO_PRIVATE_KEY) != 64) {
        ESP_LOGE(TAG, "The private key (%s) is not a 64 character long hex string.", EXAMPLE_NABTO_PRIVATE_KEY);
    }

    uint8_t key[32];

    if(!np_hex_to_data(EXAMPLE_NABTO_PRIVATE_KEY, key, sizeof(key))) {
        ESP_LOGE(TAG, "Cannot convert the private key (%s) from hex to bytes", EXAMPLE_NABTO_PRIVATE_KEY);
        return NABTO_DEVICE_EC_FAILED;
    }

    err = nabto_device_set_private_key_secp256r1(dev, key, sizeof(key));
    if (err != NABTO_DEVICE_EC_OK) {
        return err;
    }

    char* fingerprint = NULL;
    err = nabto_device_get_device_fingerprint(dev, &fingerprint);
    if (err != NABTO_DEVICE_EC_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Setting nabto private key : %s", EXAMPLE_NABTO_PRIVATE_KEY);
    ESP_LOGI(TAG, "The fingerprint for the device is: %s", fingerprint);
    nabto_device_string_free(fingerprint);


    return err;
}
