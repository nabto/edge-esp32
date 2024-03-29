#include "esp32_dns.h"

#include <platform/np_error_code.h>
#include <platform/np_ip_address.h>
#include <platform/np_completion_event.h>

#include <platform/np_logging.h>

#define LOG NABTO_LOG_MODULE_DNS

#include <lwip/dns.h>
#include <lwip/ip_addr.h>

#include <string.h>

static void esp32_async_resolve_v4(struct np_dns* obj, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent);
static void esp32_async_resolve_v6(struct np_dns* obj, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent);

void esp32_async_resolve(u8_t family, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent);

static struct np_dns_functions vtable = {
    .async_resolve_v4 = &esp32_async_resolve_v4,
    .async_resolve_v6 = &esp32_async_resolve_v6
};

struct np_dns esp32_dns_create_impl()
{
    struct np_dns obj;
    obj.mptr = &vtable;
    obj.data = NULL;
    return obj;
}

struct nm_dns_resolve_event {

    struct np_ip_address* ips;
    size_t ipsSize;
    size_t* ipsResolved;
    const char* host;
    u8_t family;
    struct np_completion_event* completionEvent;
};

void esp32_dns_resolve_cb(const char* name, const ip_addr_t* ipaddr, void* userData)
{
    struct nm_dns_resolve_event* event = userData;

    struct np_ip_address* ips = event->ips;
    struct np_completion_event* completionEvent = event->completionEvent;

    NABTO_LOG_TRACE(LOG, "esp_async_resolve callback recieved");


    if (ipaddr == NULL) {
        NABTO_LOG_TRACE(LOG, "esp_async_resolve callback - no adresses found");
        free(event);
        np_completion_event_resolve(completionEvent, NABTO_EC_UNKNOWN);
        return;

    } else {
        NABTO_LOG_TRACE(LOG, "esp_async_resolve callback - found: %s",ipaddr_ntoa(ipaddr));
        if(ipaddr->type == IPADDR_TYPE_V4) {
            memcpy(ips[0].ip.v4, &ipaddr->u_addr.ip4.addr, 4);
            ips[0].type = NABTO_IPV4;
            *event->ipsResolved = 1;
        } else if (ipaddr->type == IPADDR_TYPE_V6) {
            memcpy(ips[0].ip.v6, &ipaddr->u_addr.ip6.addr, 16);
            ips[0].type = NABTO_IPV6;
            *event->ipsResolved = 1;
        }
        free(event);
        np_completion_event_resolve(completionEvent, NABTO_EC_OK);
        return;
    }
    *event->ipsResolved = 0;
    free(event);
    np_completion_event_resolve(completionEvent, NABTO_EC_UNKNOWN);

}



void esp32_async_resolve(u8_t family, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent)
{

    NABTO_LOG_TRACE(LOG, "esp_async_resolve:%s", host);

    struct nm_dns_resolve_event* event = calloc(1,sizeof(struct nm_dns_resolve_event));
    if (event == NULL) {
        np_completion_event_resolve(completionEvent, NABTO_EC_OUT_OF_MEMORY);
        return;
    }
    event->host = host;
    event->ips = ips;
    event->ipsSize = ipsSize;
    event->ipsResolved = ipsResolved;
    event->completionEvent = completionEvent;
    event->family = family;

    ip_addr_t addr;
    err_t status = dns_gethostbyname_addrtype(host, &addr, esp32_dns_resolve_cb, event, family);
    if (status == ERR_OK) {
        *ipsResolved = 0;
        free(event);

        // callback is not going to be called.
        if(addr.type == IPADDR_TYPE_V4) {
            memcpy(ips[0].ip.v4, &addr.u_addr.ip4.addr, 4);
            ips[0].type = NABTO_IPV4;
            *ipsResolved = 1;
        } else if (addr.type == IPADDR_TYPE_V6) {
            memcpy(ips[0].ip.v6, &addr.u_addr.ip6.addr, 16);
            ips[0].type = NABTO_IPV6;
            *ipsResolved = 1;
        }
        np_completion_event_resolve(completionEvent, NABTO_EC_OK);
        return;

    }  else if (status == ERR_INPROGRESS) {
        // callback will be called.
        return;
    } else {
        free(event);
        np_completion_event_resolve(completionEvent, NABTO_EC_UNKNOWN);
    }
    return;

}


void esp32_async_resolve_v4(struct np_dns* obj, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent)
{
    esp32_async_resolve(LWIP_DNS_ADDRTYPE_IPV4, host, ips, ipsSize, ipsResolved, completionEvent);
}

void esp32_async_resolve_v6(struct np_dns* obj, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent)
{
    esp32_async_resolve(LWIP_DNS_ADDRTYPE_IPV6, host, ips, ipsSize, ipsResolved, completionEvent);
}
