/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "nabto/nabto_device.h"

#include "mdns.h"

/* The examples use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static NabtoDevice* device;

void handle_coap_get_request(NabtoDeviceCoapRequest* request, void* data)
{
    printf("Received CoAP GET request\n");
    const char* responseData = "helloWorld";
    NabtoDeviceCoapResponse* response = nabto_device_coap_create_response(request);
    nabto_device_coap_response_set_code(response, 205);
    nabto_device_coap_response_set_content_format(response, NABTO_DEVICE_COAP_CONTENT_FORMAT_TEXT_PLAIN_UTF8);
    nabto_device_coap_response_set_payload(response, responseData, strlen(responseData));
    nabto_device_coap_response_ready(response);
}

void handle_coap_post_request(NabtoDeviceCoapRequest* request, void* data)
{
    const char* responseData = "helloWorld";
    uint16_t contentFormat;
    NabtoDeviceCoapResponse* response = nabto_device_coap_create_response(request);
    nabto_device_coap_request_get_content_format(request, &contentFormat);
    if (contentFormat != NABTO_DEVICE_COAP_CONTENT_FORMAT_TEXT_PLAIN_UTF8) {
        const char* responseData = "Invalid content format";
        printf("Received CoAP POST request with invalid content format\n");
        nabto_device_coap_response_set_code(response, 400);
        nabto_device_coap_response_set_payload(response, responseData, strlen(responseData));
        nabto_device_coap_response_ready(response);
    } else {
        char* payload = (char*)malloc(1500);
        size_t payloadLength;
        nabto_device_coap_request_get_payload(request, (void**)&payload, &payloadLength);
        printf("Received CoAP POST request with a %d byte payload: \n%s", payloadLength, payload);
        nabto_device_coap_response_set_code(response, 205);
        nabto_device_coap_response_set_payload(response, responseData, strlen(responseData));
        nabto_device_coap_response_set_content_format(response, NABTO_DEVICE_COAP_CONTENT_FORMAT_TEXT_PLAIN_UTF8);
        nabto_device_coap_response_ready(response);
    }
}

void start_nabto_device()
{
    NabtoDeviceError ec;
    device = nabto_device_new();
    if (!device) {
        printf("nabto device new failed\n");
        return;
    }
    ec = nabto_device_set_private_key(device, "");
    if (ec) {
        printf("set private key failed\n");
        return;
    }
    ec = nabto_device_set_server_url(device, "");
    if (ec) {
        printf("set device url failed\n");
        return;
    }
    char fingerprint[33];
    memset(fingerprint, 0, 33);
    ec = nabto_device_get_device_fingerprint_hex(device, fingerprint);
    if (ec != NABTO_DEVICE_EC_OK) {
        printf("get fingerprint failed\n");
        return;
    }

    ec = nabto_device_start(device);
    if (ec != NABTO_DEVICE_EC_OK) {
        printf("failed to start device\n");
        return;
    }

    nabto_device_coap_add_resource(device, NABTO_DEVICE_COAP_GET, "/test/get", &handle_coap_get_request, NULL);
    nabto_device_coap_add_resource(device, NABTO_DEVICE_COAP_POST, "/test/post", &handle_coap_post_request, NULL);
}

void start_mdns_service()
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set("my-esp32");
    //set default instance
    mdns_instance_name_set("Jhon's ESP32 Thing");
}

void add_mdns_services()
{
    //add our services
    mdns_service_add(NULL, "_nabto", "_udp", 4433, NULL, 0);

    //NOTE: services must be added before their properties can be set
    //use custom instance for the web server
    mdns_service_instance_name_set("_nabto", "_udp", "Jhon's ESP32 Web Server");

    mdns_txt_item_t serviceTxtData[3] = {
        {"board","esp32"},
        {"productid","pr-zieg9fvm"},
        {"deviceid","de-wiqj7dce"}
    };
    //set txt data for service (will free and replace current data)
    mdns_service_txt_set("_nabto", "_udp", serviceTxtData, 3);
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    start_mdns_service();
    add_mdns_services();
    start_nabto_device();
}
