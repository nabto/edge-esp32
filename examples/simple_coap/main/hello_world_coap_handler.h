#pragma once

#include <nabto/nabto_device.h>

struct hello_world_coap_handler {
    NabtoDeviceListener* listener;
    NabtoDeviceFuture* future;
    NabtoDeviceCoapRequest* request;
};

void hello_world_coap_handler_init(struct hello_world_coap_handler* handler, NabtoDevice* device);
