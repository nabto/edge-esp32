#include "esp32_mdns.h"
#include "mdns.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_log.h"

#include <platform/interfaces/np_mdns.h>
#include <platform/np_util.h>

#include <nn/string_map.h>
#include <nn/string_set.h>

#include <string.h>

#define TAG "esp32_mdns"

static void publish_service(struct np_mdns* obj, uint16_t port, const char* instanceName, struct nn_string_set* subtypes, struct nn_string_map* txtItems);


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

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    char macString[13];
    memset(macString, 0, 13);
    np_data_to_hex(mac, 6, macString);

    mdns_hostname_set(macString);
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
    obj.mptr = &vtable;
    obj.data = NULL;
    return obj;
}

void publish_service(struct np_mdns* obj, uint16_t port, const char* instanceName, struct nn_string_set* subtypes, struct nn_string_map* txtItems)
{
    esp_err_t err;

    err = mdns_service_add(NULL, "_nabto", "_udp", port, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not add nabto mdns service");
    }

    struct nn_string_map_iterator it;
    NN_STRING_MAP_FOREACH(it, txtItems) {
        const char* key = nn_string_map_key(&it);
        const char* value = nn_string_map_value(&it);
        esp_err_t err = mdns_service_txt_item_set("_nabto", "_udp", key, value);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not add mdns txt item");
        }
    }

    const char* subtype;
    NN_STRING_SET_FOREACH(subtype, subtypes) {
        esp_err_t err = mdns_service_subtype_add_for_host(NULL, "_nabto", "_udp", NULL, subtype);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not add mdns subtype");
        }
    }
}
