#include "esp32_mdns.h"

#include <platform/np_error_code.h>
#include <platform/np_ip_address.h>
#include <platform/np_completion_event.h>


#include <lwip/dns.h>
#include <lwip/ip_addr.h>

#include <string.h>



// Empty.. we don't need this for esp32 which already is async
struct np_dns_resolver {
    bool stopped;
};

np_error_code mdns_create(struct np_platform* pl,
                          const char* productId, const char* deviceId,
                          np_mdns_get_port getPort, void* getPortUserData,
                          struct np_mdns_context** mdns) {
    return NABTO_EC_OK;
}

/**
 * Destroy a mdns server
 */
void mdns_destroy(struct np_mdns_context* mdns) {
}

/**
 * Start the mdns server
 */
void mdns_start(struct np_mdns_context* mdns) {
}

/**
 * Stop the mdns server
 */
void mdns_stop(struct np_mdns_context* mdns) {
}


void esp32_mdns_init(struct np_platform* pl)
{
    pl->mdns.create = &mdns_create;
    pl->mdns.destroy = &mdns_destroy;
    pl->mdns.start = &mdns_start;
    pl->mdns.stop = &mdns_stop;
    
}




