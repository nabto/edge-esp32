
#include "esp32_logging.h"
#include "esp32_dns.h"
#include "esp32_mdns.h"
#include "nm_select_unix.h"
#include "esp32_event_queue.h"

#include <api/nabto_device_platform.h>
#include <api/nabto_device_threads.h>
#include <api/nabto_device_integration.h>

#include <modules/timestamp/unix/nm_unix_timestamp.h>
#include <modules/unix/nm_unix_local_ip.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

struct select_unix_platform
{
    /**
     * a reference to the platform. The np_platform is owned by the NabtoDevice object.
     */
    struct np_platform* pl;

    /**
     * The select unix module.
     */
    struct nm_select_unix selectUnix;

    /**
     * The select unix event queue which is implemented specifically
     * for this implementation. The purpose of the event queue is to
     * execute events.
     */
    struct esp32_event_queue eventQueue;
};

/**
 * This function is called when nabto_device_new is invoked. This
 * function should initialize all the platform modules whis is needed
 * to create a functional platform for the device. See
 * <platform/np_platform.h> for a list of modules required for a
 * platform.
 */
np_error_code nabto_device_platform_init(struct nabto_device_context* device, struct nabto_device_mutex* coreMutex)
{

    printf("Hi from 'nabto_device_init_platform'\n");

    struct select_unix_platform* platform = calloc(1, sizeof(struct select_unix_platform));
    if (platform == NULL) {
        return NABTO_EC_OUT_OF_MEMORY;
    }

    nabto_device_integration_set_platform_data(device, platform);

    nm_select_unix_init(&platform->selectUnix);
    nm_select_unix_run(&platform->selectUnix);

    struct np_dns dnsImpl = esp32_dns_create_impl();
    struct np_timestamp timestampImpl = nm_unix_ts_get_impl();
    struct np_local_ip localIpImpl = nm_unix_local_ip_get_impl();
    struct np_udp udpImpl = nm_select_unix_udp_get_impl(&platform->selectUnix);
    struct np_tcp tcpImpl = nm_select_unix_tcp_get_impl(&platform->selectUnix);


    esp32_event_queue_init(&platform->eventQueue, coreMutex, &timestampImpl);
    esp32_event_queue_run(&platform->eventQueue);
    struct np_event_queue eventQueueImpl = esp32_event_queue_get_impl(&platform->eventQueue);

    esp32_mdns_start();
    struct np_mdns mdnsImpl = esp32_mdns_get_impl();

    nabto_device_integration_set_dns_impl(device, &dnsImpl);
    nabto_device_integration_set_timestamp_impl(device, &timestampImpl);
    nabto_device_integration_set_local_ip_impl(device, &localIpImpl);

    nabto_device_integration_set_tcp_impl(device, &tcpImpl);
    nabto_device_integration_set_udp_impl(device, &udpImpl);

    nabto_device_integration_set_event_queue_impl(device, &eventQueueImpl);
    nabto_device_integration_set_mdns_impl(device, &mdnsImpl);
    return NABTO_EC_OK;
}

/**
 * This function is called from nabto_device_free.
 */
void nabto_device_platform_deinit(struct nabto_device_context* device)
{
    struct select_unix_platform* platform = nabto_device_integration_get_platform_data(device);
    nm_select_unix_deinit(&platform->selectUnix);
    esp32_event_queue_deinit(&platform->eventQueue);
    free(platform);
}

/**
 * This function is called from nabto_device_stop or nabto_device_free
 * if the device is freed without being stopped first.
 */
void nabto_device_platform_stop_blocking(struct nabto_device_context* device)
{
    struct select_unix_platform* platform = nabto_device_integration_get_platform_data(device);
    nm_select_unix_stop(&platform->selectUnix);
    esp32_event_queue_stop_blocking(&platform->eventQueue);
}
