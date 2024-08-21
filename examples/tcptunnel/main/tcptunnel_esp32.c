#include <nabto/nabto_device.h>
#include <nn/log.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nabto_esp32_example.h"
#include "nabto_esp32_iam.h"
#include "tcptunnel_iam.h"
#include "device_event_handler.h"
#include "connection_event_handler.h"

#include "nabto_esp32_util.h"
#include "simple_webserver.h"
#include "simple_perf.h"

// This does not work in ESP IDF 5.0
// #include "print_stats.h"

static const char* TAG = "TcpTunnel";
static const char* LOGM = "nabto_core";

#define CHECK_NABTO_ERR(err) do { if (err != NABTO_DEVICE_EC_OK) { ESP_LOGE(TAG, "Unexpected error at %s:%d, %s", __FILE__, __LINE__, nabto_device_error_get_message(err) ); esp_restart(); } } while (0)
#define CHECK_NULL(ptr) do { if (ptr == NULL) { ESP_LOGE(TAG, "Unexpected out of memory at %s:%d", __FILE__, __LINE__); esp_restart(); } } while (0)

struct use_all_memory_item {
    struct nn_llist_node node;
    uint8_t buffer[1024];
};

void logCallback(NabtoDeviceLogMessage* msg, void* data)
{
    if (msg->severity == NABTO_DEVICE_LOG_ERROR) {
        ESP_LOGE(LOGM, "%s", msg->message);
    } else if (msg->severity == NABTO_DEVICE_LOG_WARN) {
        ESP_LOGW(LOGM, "%s", msg->message);
    } else if (msg->severity == NABTO_DEVICE_LOG_INFO) {
        ESP_LOGI(LOGM, "%s", msg->message);
    } else if (msg->severity == NABTO_DEVICE_LOG_TRACE) {
        ESP_LOGV(LOGM, "%s", msg->message);
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nabto_esp32_example_init_wifi();

    // esp_log_level_set(LOGM, ESP_LOG_VERBOSE);
    // esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    // esp_log_level_set("iam", ESP_LOG_VERBOSE);

    nvs_handle_t nvsHandle;
    ret = nvs_open("nabto", NVS_READWRITE, &nvsHandle);
    ESP_ERROR_CHECK(ret);

    //xTaskCreatePinnedToCore(stats_task, "stats", 4096, NULL, STATS_TASK_PRIO, NULL, tskNO_AFFINITY);

    NabtoDevice* dev = nabto_device_new();
    CHECK_NULL(dev);

    nabto_device_set_log_callback(dev, logCallback, NULL);
    CHECK_NABTO_ERR(nabto_device_set_log_level(dev, "trace"));

    //nabto_device_set_basestation_attach(dev, false);

    //nabto_esp32_example_load_private_key(dev, nvsHandle);

    CHECK_NABTO_ERR(nabto_esp32_example_set_id_and_key(dev));

    CHECK_NABTO_ERR(nabto_device_enable_mdns(dev));
    CHECK_NABTO_ERR(nabto_device_mdns_add_subtype(dev, "tcptunnel"));
    CHECK_NABTO_ERR(nabto_device_mdns_add_txt_item(dev, "fn", "tcptunnel"));

    CHECK_NABTO_ERR(nabto_device_set_app_name(dev, "Tcp Tunnel"));

    struct nn_log logger;
    nabto_esp32_util_nn_log_init(&logger);

    struct nabto_esp32_iam iam;

    struct nm_iam_state* defaultIamState = tcptunnel_create_default_iam_state(dev);
    struct nm_iam_configuration* iamConfig = tcptunnel_create_iam_config();

    nabto_esp32_iam_init(&iam, dev, iamConfig, defaultIamState, nvsHandle);

    nm_iam_state_free(defaultIamState);

    // Either the webserver or the perf server can be enabled but not both at the same time, the reason is currently not known.

    start_webserver(&logger);
    CHECK_NABTO_ERR(nabto_device_add_tcp_tunnel_service(dev, "http", "http", "127.0.0.1", 80));

    //xTaskCreate(&perf_task, "perf_task", 4096, NULL, 5, NULL);
    //CHECK_NABTO_ERR(nabto_device_add_tcp_tunnel_service(dev, "perf", "perf", "127.0.0.1", 9000));


    CHECK_NABTO_ERR(nabto_device_limit_connections(dev, 2));
    CHECK_NABTO_ERR(nabto_device_limit_stream_segments(dev, 80));

    NabtoDeviceFuture* future = nabto_device_future_new(dev);
    CHECK_NULL(future);

    nabto_device_start(dev, future);
    CHECK_NABTO_ERR(nabto_device_future_wait(future));
    nabto_device_future_free(future);

    char* fingerprint = NULL;
    CHECK_NABTO_ERR(nabto_device_get_device_fingerprint(dev, &fingerprint));

    ESP_LOGI(TAG, "Started nabto device with fingerprint %s", fingerprint);
    nabto_device_string_free(fingerprint);
    ESP_LOGI(TAG, "Pairring string:%s", tcptunnel_iam_create_pairing_string(&iam.iam, CONFIG_EXAMPLE_NABTO_PRODUCT_ID,CONFIG_EXAMPLE_NABTO_DEVICE_ID));

    struct device_event_handler eh;
    device_event_handler_init(&eh, dev);

    struct connection_event_handler ceh;
    connection_event_handler_init(&ceh, dev);

    for (int i = 0;; i++) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        //heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        fflush(stdout);
    }

    esp_restart();
}
