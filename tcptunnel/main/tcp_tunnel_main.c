
// Nabto includes
#include "iam_config.h"
#include "tcp_tunnel_state.h"
#include "device_event_handler.h"

#include "private_key.h"

#include <nabto/nabto_device.h>
#include "device_config.h"
#include "logging.h"

#include <modules/iam/nm_iam.h>
#include <modules/iam/nm_iam_user.h>

#include <nn/string_set.h>
#include <nn/log.h>


// ESP-IDF includes
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


// General includes
#include <stdio.h>
#include <stdlib.h>

#define NEWLINE "\n"

#ifdef CONFIG_IDF_TARGET_ESP32
#define CHIP_NAME "ESP32"
#endif

#ifdef CONFIG_IDF_TARGET_ESP32S2BETA
#define CHIP_NAME "ESP32-S2 Beta"
#endif




/**
 * Setups .. you can set it up in the menuconfig or override them here
 */
#define EXAMPLE_ESP_WIFI_SSID           CONFIG_SSID
#define EXAMPLE_ESP_WIFI_PASS           CONFIG_SSID_PASSWORD

#define EXAMPLE_NABTO_PRODUCT           CONFIG_NABTO_PRODUCT
#define EXAMPLE_NABTO_DEVICE            CONFIG_NABTO_DEVICE

#define EXAMPLE_ESP_MAXIMUM_RETRY  10

#define LOG_LEVEL "trace"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;


enum {
    OPTION_HELP = 1,
    OPTION_VERSION,
    OPTION_LOG_LEVEL,
    OPTION_SHOW_STATE,
    OPTION_HOME_DIR
};

struct args {
    bool showHelp;
    bool showVersion;
    bool showState;
    const char* logLevel;
    char* homeDir;
};


struct tcp_tunnel {
    char* pairingPassword;
    char* pairingServerConnectToken;
};


NabtoDevice* device_;

static void print_iam_state(struct nm_iam* iam);
static void iam_user_changed(struct nm_iam* iam, const char* id, void* userData);



void print_version()
{
    printf("TCP Tunnel Device Version: %s" NEWLINE, nabto_device_version());
}

void print_device_config_load_failed(const char* fileName)
{
    printf("Could not open or parse the device config file (%s)." NEWLINE, fileName);
    printf("Please ensure the file exists and has the following format." NEWLINE);
    printf("{" NEWLINE);
    printf("  \"ProductId\": \"pr-abcd1234\"," NEWLINE);
    printf("  \"DeviceId\": \"de-abcd1234\"," NEWLINE);
    printf("  \"Server\": \"pr-abcd1234.devices.nabto.net or pr-abcd1234.devices.dev.nabto.net or something else.\"," NEWLINE);
    printf("  \"client\": {" NEWLINE);
    printf("    \"ServerKey\": \"sk-...\"," NEWLINE);
    printf("    \"ServerUrl\": \"https://pr-abcd1234.clients.dev.nabto.net or https://pr-abcd1234.clients.nabto.net or something else\"," NEWLINE);
    printf("  }" NEWLINE);
    printf("}" NEWLINE);
}

void print_iam_config_load_failed(const char* fileName)
{
    printf("Could not open or parse IAM config file (%s)" NEWLINE, fileName);
}

void print_tcp_tunnel_state_load_failed(const char* fileName)
{
    printf("Could not load TCP tunnel state file (%s)" NEWLINE, fileName);
}

void print_start_text(struct args* args)
{
    printf("TCP Tunnel Device" NEWLINE);
}

void print_private_key_file_load_failed(const char* fileName)
{
    printf("Could not load the private key (%s) see error log for further details." NEWLINE, fileName);
}

bool check_log_level(const char* level)
{
    if (strcmp(level, "error") == 0 ||
        strcmp(level, "warn") == 0 ||
        strcmp(level, "info") == 0 ||
        strcmp(level, "trace") == 0)
    {
        return true;
    }
    return false;
}


void tcp_tunnel_init(struct tcp_tunnel* tunnel)
{
    memset(tunnel, 0, sizeof(struct tcp_tunnel));
}

void tcp_tunnel_deinit(struct tcp_tunnel* tunnel)
{

    free(tunnel->pairingPassword);
    free(tunnel->pairingServerConnectToken);
}

char* generate_pairing_url(const char* productId, const char* deviceId, const char* deviceFingerprint,
                               const char* clientServerUrl, const char* clientServerKey,
                               const char* pairingPassword, const char* pairingServerConnectToken) {
char* buffer = calloc(1, 1024); // long enough!

sprintf(buffer, "https://tcp-tunnel.nabto.com/pairing?ProductId=%s&DeviceId=%s&DeviceFingerprint=%s&ClientServerUrl=%s&ClientServerKey=%s&PairingPassword=%s&ClientServerConnectToken=%s",
            productId,
            deviceId,
            deviceFingerprint,
            clientServerUrl,
            clientServerKey,
            pairingPassword,
            pairingServerConnectToken);

return buffer;
}

void print_item(const char* item)
{
    size_t printSize = strlen(item);
    if (printSize > 16) {
        printSize = 16;
    }
    printf("%.*s", (int)printSize, item);

    const char* spaces = "                 ";
    size_t spacesSize = 17 - printSize;
    printf("%.*s", (int)spacesSize, spaces);
}

bool handle_main(struct tcp_tunnel* tunnel);


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





int app_main(void)
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


    // wait for connection
    printf("Main task: waiting for connection to the wifi network... ");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    printf("connected!\n");

    // print the local IP address
    tcpip_adapter_ip_info_t ip_info;
    ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));

    printf("IP Address:  %s_wifi_event_group\n", ip4addr_ntoa(&ip_info.ip));
    printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
    printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));



    struct tcp_tunnel tunnel;
    tcp_tunnel_init(&tunnel);

    bool status = handle_main(&tunnel);

    tcp_tunnel_deinit(&tunnel);


    if (status) {
        return 0;
    } else {
        return 1;
    }
    return 1;

}

bool handle_main(struct tcp_tunnel* tunnel)
{


    NabtoDevice* device = nabto_device_new();

    struct nn_log logger;
    logging_init(device, &logger, LOG_LEVEL);


    /**
     * Load data files
     */
    struct device_config dc;
    device_config_init(&dc);


    if (!load_device_config_esp32(&dc, &logger)) {
        printf("Failed to start device. Could not load device config");
        return false;
    }

    struct tcp_tunnel_state tcpTunnelState;
    tcp_tunnel_state_init(&tcpTunnelState);

    if (!load_tcp_tunnel_state(&tcpTunnelState, &logger)) {
        print_tcp_tunnel_state_load_failed("ESP32-NVS storage");
        return false;
    }


    if (!load_or_create_private_key_esp32nvs(device, &logger)) {
        print_private_key_file_load_failed("ESP32-NVS storage");
        return false;
    }


    nabto_device_set_product_id(device, dc.productId);
    nabto_device_set_device_id(device, dc.deviceId);
    nabto_device_set_server_url(device, dc.server);
    nabto_device_enable_mdns(device);


    struct nm_iam iam;
    nm_iam_init(&iam, device, &logger);

    if (tcpTunnelState.pairingPassword != NULL) {
        nm_iam_enable_password_pairing(&iam, tcpTunnelState.pairingPassword);
        tunnel->pairingPassword = strdup(tcpTunnelState.pairingPassword);
    }

    if (tcpTunnelState.pairingServerConnectToken != NULL) {
        nm_iam_enable_remote_pairing(&iam, tcpTunnelState.pairingServerConnectToken);
        tunnel->pairingServerConnectToken = strdup(tcpTunnelState.pairingServerConnectToken);
    }

    nm_iam_enable_client_settings(&iam, dc.clientServerUrl, dc.clientServerKey);


    // Add a tunnel service to localhost port 80 .. ie. make tunnels to the local webserver possible
    nabto_device_add_tcp_tunnel_service(device, "http", "http", "127.0.0.1", 80);

    char* deviceFingerprint;
    nabto_device_get_device_fingerprint_full_hex(device, &deviceFingerprint);
    
    char* pairingUrl = generate_pairing_url(dc.productId, dc.deviceId, deviceFingerprint, dc.clientServerUrl, dc.clientServerKey, tcpTunnelState.pairingPassword, tcpTunnelState.pairingServerConnectToken);

    load_iam_config(&iam);

    // add users to iam module.
    struct nm_iam_user* user;
    NN_VECTOR_FOREACH(&user, &tcpTunnelState.users)
    {
        nm_iam_add_user(&iam, user);
    }
    nn_vector_clear(&tcpTunnelState.users);

    printf("######## Nabto TCP Tunnel Device ########" NEWLINE);
    printf("# Product ID:        %s" NEWLINE, dc.productId);
    printf("# Device ID:         %s" NEWLINE, dc.deviceId);
    printf("# Fingerprint:       %s" NEWLINE, deviceFingerprint);
    printf("# Pairing password:  %s" NEWLINE, tcpTunnelState.pairingPassword);
    printf("# Paring SCT:        %s" NEWLINE, tcpTunnelState.pairingServerConnectToken);
    printf("# Client Server Url: %s" NEWLINE, dc.clientServerUrl);
    printf("# Client Server Key: %s" NEWLINE, dc.clientServerKey);
    printf("# Version:           %s" NEWLINE, nabto_device_version());
    printf("# Pairing URL:       %s" NEWLINE, pairingUrl);

    // Next two are only strings made for printing so free them..
    free(pairingUrl);
    nabto_device_string_free(deviceFingerprint);

    struct device_event_handler eventHandler;
    device_event_handler_init(&eventHandler, device);

    print_iam_state(&iam);
    nm_iam_set_user_changed_callback(&iam, iam_user_changed, tunnel);

    nabto_device_start(device);
    nm_iam_start(&iam);

    device_ = device;


    // block until the NABTO_DEVICE_EVENT_CLOSED event is emitted.
    device_event_handler_blocking_listener(&eventHandler);


    nabto_device_stop(device);

    tcp_tunnel_state_deinit(&tcpTunnelState);

    device_event_handler_deinit(&eventHandler);


    nabto_device_stop(device);
    nm_iam_deinit(&iam);
    nabto_device_free(device);


    device_config_deinit(&dc);

    return true;
}



void print_iam_state(struct nm_iam* iam)
{
    struct nn_string_set ss;
    nn_string_set_init(&ss);
    nm_iam_get_users(iam, &ss);

    const char* id;
    NN_STRING_SET_FOREACH(id, &ss)
    {
        struct nm_iam_user* user = nm_iam_find_user(iam, id);
        printf("User: %s, fingerprint: %s" NEWLINE, user->id, user->fingerprint);
    }
    nn_string_set_deinit(&ss);
}


void iam_user_changed(struct nm_iam* iam, const char* id, void* userData)
{
    struct tcp_tunnel* tcpTunnel = userData;

    struct tcp_tunnel_state toWrite;

    tcp_tunnel_state_init(&toWrite);
    if (tcpTunnel->pairingPassword) {
        toWrite.pairingPassword = strdup(tcpTunnel->pairingPassword);
    }
    if (tcpTunnel->pairingServerConnectToken) {
        toWrite.pairingServerConnectToken = strdup(tcpTunnel->pairingServerConnectToken);
    }

    struct nn_string_set userIds;
    nn_string_set_init(&userIds);
    nm_iam_get_users(iam, &userIds);

    const char* uid;
    NN_STRING_SET_FOREACH(uid, &userIds)
    {
        struct nm_iam_user* user = nm_iam_find_user(iam, uid);
        nn_vector_push_back(&toWrite.users, &user);
    }
    nn_string_set_deinit(&userIds);
    save_tcp_tunnel_state(&toWrite);
    tcp_tunnel_state_deinit(&toWrite);
}
