#include "esp32_mdns.h"
#include "mdns.h"

#include <platform/np_error_code.h>
#include <platform/np_ip_address.h>
#include <platform/np_completion_event.h>
#include <platform/interfaces/np_mdns.h>


#include <lwip/dns.h>
#include <lwip/ip_addr.h>

#include <string.h>


static void publish_service(struct np_mdns* obj, uint16_t port, const char* productId, const char* deviceId);

void esp32_mdns_start()
{
    printf("starting mdns server\n");
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set("my-esp32");
}

void esp32_mdns_stop()
{
    // TODO
}

static struct np_mdns_functions vtable = {
    .publish_service = &publish_service
};

struct np_mdns esp32_mdns_get_impl()
{
    struct np_mdns obj;
    obj.vptr = &vtable;
    obj.data = NULL;
    return obj;
}


void publish_service(struct np_mdns* obj, uint16_t port, const char* productId, const char* deviceId)
{
    printf("register mdns service\n");
    mdns_service_add(NULL, "_nabto", "_udp", port, NULL, 0);

    mdns_txt_item_t serviceTxtData[2] = {
        {"productid",productId},
        {"deviceid",deviceId}
    };
    //set txt data for service (will free and replace current data)
    mdns_service_txt_set("_nabto", "_udp", serviceTxtData, 2);
}
