#include "hello_world_coap_handler.h"

static void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData);
static void start_listen(struct hello_world_coap_handler* handler);

void hello_world_coap_handler_init(struct hello_world_coap_handler* handler, NabtoDevice* device)
{
    handler->future = nabto_device_future_new(device);
    handler->listener = nabto_device_listener_new(device);

    const char* pathSegments[] = { "hello-world", NULL };

    nabto_device_coap_init_listener(device, handler->listener, NABTO_DEVICE_COAP_GET, pathSegments );

    start_listen(handler);
}

void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData)
{
    if (ec == NABTO_DEVICE_EC_OK) {
        struct hello_world_coap_handler* handler = userData;

        nabto_device_coap_response_set_code(handler->request, 205);
        const char* hello = "HelloWorld!";
        nabto_device_coap_response_set_payload(handler->request, hello, strlen(hello));
        nabto_device_coap_response_set_content_format(handler->request, NABTO_DEVICE_COAP_CONTENT_FORMAT_TEXT_PLAIN_UTF8);
        nabto_device_coap_response_ready(handler->request);
        nabto_device_coap_request_free(handler->request);

        start_listen(handler);
    }
}

void start_listen(struct hello_world_coap_handler* handler) {
    nabto_device_listener_new_coap_request(handler->listener, handler->future, &handler->request);
    nabto_device_future_set_callback(handler->future, callback, handler);
}
