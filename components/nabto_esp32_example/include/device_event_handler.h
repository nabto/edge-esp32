#ifndef _DEVICE_EVENT_HANDLER_H_
#define _DEVICE_EVENT_HANDLER_H_

#include <nabto/nabto_device.h>

struct device_event_handler {
    NabtoDevice* device;
    NabtoDeviceFuture* future;
    NabtoDeviceListener* listener;
    NabtoDeviceEvent event;
};


void device_event_handler_init(struct device_event_handler* handler, NabtoDevice* device);


#endif
