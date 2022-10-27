#include "device_event_handler.h"

#include "esp_log.h"

static const char* TAG = "device_event_handler";

static void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData);
static void handle_event(struct device_event_handler* handler, NabtoDeviceEvent event);

void device_event_handler_start_listen(struct device_event_handler* handler)
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
    device_event_handler_start_listen(handler);
}

void handle_event(struct device_event_handler* handler, NabtoDeviceEvent event)
{
    if (event == NABTO_DEVICE_EVENT_ATTACHED) {
        ESP_LOGI(TAG, "Attached to the basestation");
    } else if (event == NABTO_DEVICE_EVENT_DETACHED) {
        ESP_LOGI(TAG, "Detached from the basestation");
    } else if (event == NABTO_DEVICE_EVENT_UNKNOWN_FINGERPRINT) {
        char* fingerprint;
        nabto_device_get_device_fingerprint(handler->device, &fingerprint);
        ESP_LOGI(TAG, "The fingerprint %s is not known by the Nabto Edge Cloud. fix it on https://console.cloud.nabto.com", fingerprint);
        nabto_device_string_free(fingerprint);
    }
}

void device_event_handler_init(struct device_event_handler* handler, NabtoDevice* device)
{
    handler->device = device;
    handler->listener = nabto_device_listener_new(device);
    handler->future = nabto_device_future_new(device);
    nabto_device_device_events_init_listener(device, handler->listener);

    device_event_handler_start_listen(handler);
}
