#include "private_key.h"

#include "nvs_flash.h"
#include "esp_system.h"

#include <stdio.h>
#include <stdlib.h>

static const char* LOGM = "private_key";

static const char* PRIVATEKEY = "private_key";



bool load_or_create_private_key_esp32nvs(NabtoDevice* device, struct nn_log* logger)
{
    size_t keyLength;
    nvs_handle_t handle;
    nvs_open("nabto", NVS_READWRITE, &handle);
    
    esp_err_t status = nvs_get_str(handle, PRIVATEKEY, NULL, &keyLength);
    if (status != ESP_OK || keyLength > 512) {
        char* key;
        nabto_device_create_private_key(device, &key);
        nabto_device_set_private_key(device, key);
        nvs_set_str(handle, "private_key", key);
        nabto_device_string_free(key);
    } else {
        // read key from nvs
        char* key = malloc(512);
        status = nvs_get_str(handle, PRIVATEKEY, key, &keyLength);
        if (status == ESP_OK) {
            nabto_device_set_private_key(device, key);
        } else {
            NN_LOG_INFO(logger, LOGM, "Cannot read private key");
            esp_restart();
            return false;
        }
    }
    nvs_close(handle);
    return true;
}
