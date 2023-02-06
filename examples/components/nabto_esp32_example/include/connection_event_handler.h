#ifndef _CONNECTION_EVENT_HANDLER_H_
#define _CONNECTION_EVENT_HANDLER_H_

#include <nabto/nabto_device.h>

struct connection_event_handler {
    NabtoDevice* device;
    NabtoDeviceFuture* future;
    NabtoDeviceListener* listener;
    NabtoDeviceConnectionEvent connectionEvent;
    NabtoDeviceConnectionRef connectionRef;
};


void connection_event_handler_init(struct connection_event_handler* handler, NabtoDevice* device);


#endif
