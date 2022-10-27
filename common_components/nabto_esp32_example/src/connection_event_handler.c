#include "connection_event_handler.h"

#include "esp_log.h"

static const char* TAG = "connection_event_handler";

static void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData);
static void handle_event(struct connection_event_handler* handler, NabtoDeviceConnectionRef connectionRef, NabtoDeviceConnectionEvent connectionEvent);
static void start_listen(struct connection_event_handler* handler);

void start_listen(struct connection_event_handler* handler)
{
    nabto_device_listener_connection_event(handler->listener, handler->future, &handler->connectionRef, &handler->connectionEvent);
    nabto_device_future_set_callback(handler->future, &callback, handler);
}

void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData)
{
    struct connection_event_handler* handler = userData;
    if (ec != NABTO_DEVICE_EC_OK) {
        return;
    }
    handle_event(handler, handler->connectionRef, handler->connectionEvent);
    start_listen(handler);
}

void handle_event(struct connection_event_handler* handler, NabtoDeviceConnectionRef connectionRef, NabtoDeviceConnectionEvent connectionEvent)
{
    if (connectionEvent == NABTO_DEVICE_CONNECTION_EVENT_OPENED) {
        ESP_LOGI(TAG, "A connection has been opened.");
    } else if (connectionEvent == NABTO_DEVICE_CONNECTION_EVENT_CLOSED) {
        ESP_LOGI(TAG, "A connection has been closed.");
    }
}

void connection_event_handler_init(struct connection_event_handler* handler, NabtoDevice* device)
{
    handler->device = device;
    handler->listener = nabto_device_listener_new(device);
    handler->future = nabto_device_future_new(device);
    nabto_device_connection_events_init_listener(device, handler->listener);

    start_listen(handler);
}
