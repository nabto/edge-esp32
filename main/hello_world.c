// hw
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <esp_http_server.h>

#include "lwip/err.h"
#include "lwip/sys.h"

#include <nabto/nabto_device.h>

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif

/**
 * Setups .. you can set it up in the menuconfig or override them here
 */
#define EXAMPLE_ESP_WIFI_SSID		CONFIG_SSID
#define EXAMPLE_ESP_WIFI_PASS		CONFIG_SSID_PASSWORD

#define EXAMPLE_NABTO_PRODUCT		CONFIG_NABTO_PRODUCT
#define EXAMPLE_NABTO_DEVICE		CONFIG_NABTO_DEVICE

#define EXAMPLE_ESP_MAXIMUM_RETRY  10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;


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

void wifi_init_sta(void)
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



void app_main(void)
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU cores, WiFi%s%s, ",
            CHIP_NAME,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

        //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();


    NabtoDevice* dev = nabto_device_new();
    nabto_device_set_log_std_out_callback(dev);
    nabto_device_set_log_level(dev, "trace");
    nabto_device_set_product_id(dev, EXAMPLE_NABTO_PRODUCT);
    printf("Setting nabto product id : %s\n", EXAMPLE_NABTO_PRODUCT);
    nabto_device_set_device_id(dev, EXAMPLE_NABTO_DEVICE);
    printf("Setting nabto device id : %s\n", EXAMPLE_NABTO_DEVICE);
    nabto_device_set_server_url(dev, "a.devices.dev.nabto.net");

    size_t keyLength;
    nvs_handle_t handle;
    nvs_open("nabto", NVS_READWRITE, &handle);

    esp_err_t status = nvs_get_str(handle, "private_key", NULL, &keyLength);
    if (status != ESP_OK || keyLength > 512) {
        char* key;
        nabto_device_create_private_key(dev, &key);
        nabto_device_set_private_key(dev, key);
        nvs_set_str(handle, "private_key", key);
        nabto_device_string_free(key);
    } else {
        // read key from nvs
        char* key = malloc(512);
        status = nvs_get_str(handle, "private_key", key, &keyLength);
        if (status == ESP_OK) {
            nabto_device_set_private_key(dev, key);
        } else {
            ESP_LOGI(TAG, "Cannot read private key");
            esp_restart();
            // fail
        }
    }
    nvs_close(handle);

    nabto_device_start(dev);

    char* fingerprint;
    nabto_device_get_device_fingerprint_hex(dev, &fingerprint);

    printf("Started nabto device with fingerprint %s\n", fingerprint);

    NabtoDeviceListener* listener = nabto_device_listener_new(dev);

    const char* path[] = {"test", "get", NULL };

    nabto_device_coap_init_listener(dev, listener, NABTO_DEVICE_COAP_GET, path);

    NabtoDeviceFuture* future = nabto_device_future_new(dev);

    while (true) {
        NabtoDeviceCoapRequest* request;
        nabto_device_listener_new_coap_request(listener, future, &request);
        NabtoDeviceError ec = nabto_device_future_wait(future);
        if (ec) {
            esp_restart();
        }

        nabto_device_coap_response_set_code(request, 205);
        const char* hello = "HelloWorld!";
        nabto_device_coap_response_set_payload(request, hello, strlen(hello));
        nabto_device_coap_response_set_content_format(request, NABTO_DEVICE_COAP_CONTENT_FORMAT_TEXT_PLAIN_UTF8);
        nabto_device_coap_response_ready(request);
        nabto_device_coap_request_free(request);
    }


    printf("Waiting forever\n");
    for (int i = 0; ; i++) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        fflush(stdout);
    }


    esp_restart();
}

