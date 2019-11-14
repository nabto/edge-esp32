#include "esp32_dns.h"

#include <platform/np_error_code.h>
#include <platform/np_ip_address.h>

#include <lwip/dns.h>
#include <lwip/ip_addr.h>

#include <string.h>

static np_error_code esp32_dns_resolve(struct np_platform* pl, const char* host, np_dns_resolve_callback cb, void* data);


void esp32_dns_init(struct np_platform* pl)
{
    pl->dns.async_resolve = &esp32_dns_resolve;
}

struct resolver_state {
    struct np_platform* pl;
    np_dns_resolve_callback cb;
    void* cbData;
    struct np_ip_address addr;
    struct np_event event;
    np_error_code ec;

};

void resolve_event(void* data)
{
    struct resolver_state* state = data;
    state->cb(state->ec, &state->addr, 1, NULL, 0, state->cbData);
    free(state);
}

void dns_found_cb(const char* name, const ip_addr_t* ipaddr, void* userData)
{
    struct resolver_state* state = userData;

    if (ipaddr == NULL) {
        state->ec = NABTO_EC_UNKNOWN;
    } else {
        state->addr.type = NABTO_IPV4;
        memcpy(state->addr.ip.v4, &ipaddr->u_addr.ip4.addr, 4);
    }

    np_event_queue_post(state->pl, &state->event, resolve_event, state);
}

np_error_code esp32_dns_resolve(struct np_platform* pl, const char* host, np_dns_resolve_callback cb, void* data)
{
    struct resolver_state* state = calloc(1, sizeof(struct resolver_state));

    state->pl = pl;
    state->cb = cb;
    state->cbData = data;

    ip_addr_t addr;
    err_t status = dns_gethostbyname(host, &addr, dns_found_cb, state);
    if (status == ERR_OK) {
        // callback is not going to be called.
        state->addr.type = NABTO_IPV4;
        memcpy(state->addr.ip.v4, &addr.u_addr.ip4.addr, 4);
        np_event_queue_post(pl, &state->event, resolve_event, state);
        return NABTO_EC_OK;
    } else if (status == ERR_INPROGRESS) {
        // callback will be called.
        return NABTO_EC_OK;
    } else {
        free(state);
        return NABTO_EC_UNKNOWN;
    }

}
