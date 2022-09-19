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

#include "device_event_handler.h"


#include <nabto/nabto_device.h>
#include <nn/log.h>

#include <esp32_logging.h>

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#define EXAMPLE_NABTO_PRODUCT_ID   CONFIG_NABTO_PRODUCT_ID
#define EXAMPLE_NABTO_DEVICE_ID    CONFIG_NABTO_DEVICE_ID

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void start_listen(struct device_event_handler* handler);
static void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData);
static void handle_event(struct device_event_handler* handler, NabtoDeviceEvent event);


static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void app_main(void)
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

    // wait for connection
    printf("Main task: waiting for connection to the wifi network... ");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    printf("connected!\n");

    NabtoDevice* dev = nabto_device_new();

    struct nn_log logger;
    esp32_logging_init(dev, &logger);




    nabto_device_set_product_id(dev, EXAMPLE_NABTO_PRODUCT_ID);
    printf("Setting nabto product id : %s\n", EXAMPLE_NABTO_PRODUCT_ID);
    nabto_device_set_device_id(dev, EXAMPLE_NABTO_DEVICE_ID);
    printf("Setting nabto device id : %s\n", EXAMPLE_NABTO_DEVICE_ID);

    nabto_device_enable_mdns(dev);

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

    NabtoDeviceFuture* future = nabto_device_future_new(dev);

    nabto_device_start(dev, future);
    NabtoDeviceError ec = nabto_device_future_wait(future);
    if (ec != NABTO_DEVICE_EC_OK) {
        ESP_LOGE(TAG, "ec %s", nabto_device_error_get_message(ec));
    }

    char* fingerprint;
    nabto_device_get_device_fingerprint(dev, &fingerprint);

    printf("Started nabto device with fingerprint %s\n", fingerprint);

    NabtoDeviceListener* listener = nabto_device_listener_new(dev);

    const char* path[] = {"hello-world", NULL };

    nabto_device_coap_init_listener(dev, listener, NABTO_DEVICE_COAP_GET, path);
    struct device_event_handler eventHandler;
    device_event_handler_init(&eventHandler, dev);

    while (true) {
        NabtoDeviceCoapRequest* request;
        nabto_device_listener_new_coap_request(listener, future, &request);
        NabtoDeviceError ec = nabto_device_future_wait(future);

        printf("Got request, after wait HERE HERE HERE HERE HERE HERE");

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

void start_listen(struct device_event_handler* handler)
{
    nabto_device_listener_device_event(handler->listener, handler->future, &handler->event);
    nabto_device_future_set_callback(handler->future, &callback, handler);
}

void callback(NabtoDeviceFuture* future, NabtoDeviceError ec, void* userData)
{
    struct device_event_handler* handler = userData;
    if (ec != NABTO_DEVICE_EC_OK) {
        return;
    }
    handle_event(handler, handler->event);
    start_listen(handler);
}

void handle_event(struct device_event_handler* handler, NabtoDeviceEvent event)
{
    if (event == NABTO_DEVICE_EVENT_ATTACHED) {
        printf("Attached to the basestation\n");
    } else if (event == NABTO_DEVICE_EVENT_DETACHED) {
        printf("Detached from the basestation\n");
    }
}

void device_event_handler_init(struct device_event_handler* handler, NabtoDevice* device)
{
    handler->device = device;
    handler->listener = nabto_device_listener_new(device);
    handler->future = nabto_device_future_new(device);
    nabto_device_device_events_init_listener(device, handler->listener);
}
