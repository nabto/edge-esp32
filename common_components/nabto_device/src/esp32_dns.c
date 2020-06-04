#include "esp32_dns.h"

#include <platform/np_error_code.h>
#include <platform/np_ip_address.h>
#include <platform/np_completion_event.h>

#include <platform/np_logging.h>

#define LOG NABTO_LOG_MODULE_DNS

#include <lwip/dns.h>
#include <lwip/ip_addr.h>

#include <string.h>

static void esp32_async_resolve_v4(struct np_dns_resolver* resolver, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent);
static void esp32_async_resolve_v6(struct np_dns_resolver* resolver, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent);
static np_error_code esp32_create_resolver(struct np_platform* pl, struct np_dns_resolver** resolver);
static void  esp32_destroy_resolver(struct np_dns_resolver* resolver);
static void esp32_stop_resolver(struct np_dns_resolver* resolver);

void esp32_async_resolve(struct np_dns_resolver* resolver, u8_t family, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent);

void esp32_dns_init(struct np_platform* pl)
{
    pl->dns.async_resolve_v4 = &esp32_async_resolve_v4;
    pl->dns.async_resolve_v6 = &esp32_async_resolve_v6;
    pl->dns.destroy_resolver = &esp32_destroy_resolver;
    pl->dns.create_resolver = &esp32_create_resolver;
    pl->dns.stop = &esp32_stop_resolver;
    
}

// Empty.. we don't need this for esp32 which already is async
struct np_dns_resolver {
    bool stopped;
};


struct nm_dns_resolve_event {

    struct np_ip_address* ips;
    size_t ipsSize;
    size_t* ipsResolved;
    const char* host;
    u8_t family;
    struct np_completion_event* completionEvent;
};



np_error_code esp32_create_resolver(struct np_platform* pl, struct np_dns_resolver** resolver) {
    printf("create_resolver called\n");
    struct np_dns_resolver* r = calloc(1, sizeof(struct np_dns_resolver));
    r->stopped = false;
    *resolver = r;
    
    
    return NABTO_EC_OK;
}

void esp32_destroy_resolver(struct np_dns_resolver* resolver) {
    free(resolver);
    printf("destroy_resolver called\n");
}

void esp32_stop_resolver(struct np_dns_resolver* resolver) {
    resolver->stopped = true;
    printf("stop called\n");    
}



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
        if(event->family == LWIP_DNS_ADDRTYPE_IPV4) {
            NABTO_LOG_TRACE(LOG, "esp_async_resolve callback - found: %s",ipaddr_ntoa(ipaddr));
            memcpy(ips[0].ip.v4, &ipaddr->u_addr.ip4.addr, 4);
            *event->ipsResolved = 1;
            free(event);
            np_completion_event_resolve(completionEvent, NABTO_EC_OK);
            return;
        }
    }
    free(event);
    *event->ipsResolved = 0;
    np_completion_event_resolve(completionEvent, NABTO_EC_UNKNOWN);

}



void esp32_async_resolve(struct np_dns_resolver* resolver, u8_t family, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent)
{

    NABTO_LOG_TRACE(LOG, "esp_async_resolve:%s", host);
    if (resolver->stopped) {
        np_completion_event_resolve(completionEvent, NABTO_EC_STOPPED);
        return;
    }
    struct nm_dns_resolve_event* event = calloc(1,sizeof(struct nm_dns_resolve_event));
    event->host = host;
    event->ips = ips;
    event->ipsSize = ipsSize;
    event->ipsResolved = ipsResolved;
    event->completionEvent = completionEvent;
    event->family = family;
    
    ip_addr_t addr;
    err_t status = dns_gethostbyname(host, &addr, esp32_dns_resolve_cb, event);
    if (status == ERR_OK) {
        *ipsResolved = 0;
        free(event);
        
        // callback is not going to be called.
        if(family == LWIP_DNS_ADDRTYPE_IPV4) {
            memcpy(ips[0].ip.v4, &addr.u_addr.ip4.addr, 4);
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


void esp32_async_resolve_v4(struct np_dns_resolver* resolver, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent) {

    esp32_async_resolve(resolver,LWIP_DNS_ADDRTYPE_IPV4, host, ips, ipsSize, ipsResolved, completionEvent);
    
}

void esp32_async_resolve_v6(struct np_dns_resolver* resolver, const char* host, struct np_ip_address* ips, size_t ipsSize, size_t* ipsResolved, struct np_completion_event* completionEvent) {

    np_completion_event_resolve(completionEvent, NABTO_EC_NOT_IMPLEMENTED);

    

    
}



