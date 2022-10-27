#include "nabto_init.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "device_event_handler.h"

#include <nabto/nabto_device.h>
#include <nn/log.h>

#define EXAMPLE_NABTO_PRODUCT_ID   CONFIG_NABTO_PRODUCT_ID
#define EXAMPLE_NABTO_DEVICE_ID    CONFIG_NABTO_DEVICE_ID

static const char *TAG = "nabto";

static NabtoDevice* dev = NULL;

static struct device_event_handler eventHandler;

static void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData);
static void start_listen(struct device_event_handler* handler);
static void handle_event(struct device_event_handler* handler, NabtoDeviceEvent event);

NabtoDevice* nabto_init_get_device() {
    return dev;
}

void nabto_init()
{
    dev = nabto_device_new();

    nabto_device_set_product_id(dev, EXAMPLE_NABTO_PRODUCT_ID);
    ESP_LOGI(TAG, "Setting nabto product id : %s", EXAMPLE_NABTO_PRODUCT_ID);
    nabto_device_set_device_id(dev, EXAMPLE_NABTO_DEVICE_ID);
    ESP_LOGI(TAG, "Setting nabto device id : %s", EXAMPLE_NABTO_DEVICE_ID);

    nabto_device_enable_mdns(dev);

    nabto_device_add_server_connect_token(dev, "demosct");

    size_t keyLength;
    nvs_handle_t handle;
    nvs_open("nabto", NVS_READWRITE, &handle);

    esp_err_t status = nvs_get_str(handle, "private_key", NULL, &keyLength);
    if (status != ESP_OK || keyLength > 512) {
        char* key;
        nabto_device_create_private_key(dev, &key);
        nabto_device_set_private_key(dev, key);
        nvs_set_str(handle, "private_key", key);
        nabto_device_string_free(key);
    } else {
        // read key from nvs
        char* key = malloc(512);
        status = nvs_get_str(handle, "private_key", key, &keyLength);
        if (status == ESP_OK) {
            nabto_device_set_private_key(dev, key);
        } else {
            ESP_LOGI(TAG, "Cannot read private key");
            esp_restart();
            // fail
        }
    }
    nvs_close(handle);

    NabtoDeviceFuture* future = nabto_device_future_new(dev);

    nabto_device_start(dev, future);
    NabtoDeviceError ec = nabto_device_future_wait(future);
    if (ec != NABTO_DEVICE_EC_OK) {
        ESP_LOGE(TAG, "ec %s", nabto_device_error_get_message(ec));
    }

    char* fingerprint;
    nabto_device_get_device_fingerprint(dev, &fingerprint);

    ESP_LOGI(TAG, "Started nabto device with fingerprint %s", fingerprint);

    device_event_handler_init(&eventHandler, dev);
    start_listen(&eventHandler);

}

void start_listen(struct device_event_handler* handler)
{
    nabto_device_listener_device_event(handler->listener, handler->future, &handler->event);
    nabto_device_future_set_callback(handler->future, &callback, handler);
}

void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData)
{
    struct device_event_handler* handler = userData;
    if (ec != NABTO_DEVICE_EC_OK) {
        return;
    }
    handle_event(handler, handler->event);
    start_listen(handler);
}

void handle_event(struct device_event_handler* handler, NabtoDeviceEvent event)
{
    if (event == NABTO_DEVICE_EVENT_ATTACHED) {
        ESP_LOGI(TAG, "Attached to the basestation");
    } else if (event == NABTO_DEVICE_EVENT_DETACHED) {
        ESP_LOGI(TAG, "Detached from the basestation");
    }
}

void device_event_handler_init(struct device_event_handler* handler, NabtoDevice* device)
{
    handler->device = device;
    handler->listener = nabto_device_listener_new(device);
    handler->future = nabto_device_future_new(device);
    nabto_device_device_events_init_listener(device, handler->listener);
}
