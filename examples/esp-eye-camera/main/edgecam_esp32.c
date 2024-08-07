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
#include "edgecam_iam.h"
#include "device_event_handler.h"
#include "connection_event_handler.h"
#include "esp32_perfmon.h"
#include "nabto_esp32_util.h"

static const char* TAG = "ESP-CAM";

#define CHECK_NABTO_ERR(err) do { if (err != NABTO_DEVICE_EC_OK) { ESP_LOGE(TAG, "Unexpected error at %s:%d, %s", __FILE__, __LINE__, nabto_device_error_get_message(err) ); esp_restart(); } } while (0)
#define CHECK_NULL(ptr) do { if (ptr == NULL) { ESP_LOGE(TAG, "Unexpected out of memory at %s:%d", __FILE__, __LINE__); esp_restart(); } } while (0)

struct use_all_memory_item {
    struct nn_llist_node node;
    uint8_t buffer[1024];
};

static const char* LOGM = "nabto";
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

// from esp32cam.cpp
extern void rtsp_server(void*);

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

    TaskHandle_t xHandle_RTSP_Server = NULL;


    nvs_handle_t nvsHandle;
    ret = nvs_open("nabto", NVS_READWRITE, &nvsHandle);
    ESP_ERROR_CHECK(ret);

    NabtoDevice* dev = nabto_device_new();
    CHECK_NULL(dev);

    nabto_device_set_log_callback(dev, logCallback, NULL);
    CHECK_NABTO_ERR(nabto_device_set_log_level(dev, "trace"));

    CHECK_NABTO_ERR(nabto_esp32_example_set_id_and_key(dev));

    CHECK_NABTO_ERR(nabto_device_enable_mdns(dev));
    CHECK_NABTO_ERR(nabto_device_mdns_add_subtype(dev, "tcptunnel"));
    CHECK_NABTO_ERR(nabto_device_mdns_add_txt_item(dev, "fn", "esp32-cam"));

    CHECK_NABTO_ERR(nabto_device_set_app_name(dev, "Tcp Tunnel"));

    struct nn_log logger;
    nabto_esp32_util_nn_log_init(&logger);

    struct nabto_esp32_iam iam;

    struct nm_iam_state* defaultIamState = tcptunnel_create_default_iam_state(dev);
    struct nm_iam_configuration* iamConfig = tcptunnel_create_iam_config();

    nabto_esp32_iam_init(&iam, dev, iamConfig, defaultIamState, nvsHandle);

    CHECK_NABTO_ERR(nabto_device_add_tcp_tunnel_service(dev, "rtsp", "rtsp", "127.0.0.1", 8554));
    CHECK_NABTO_ERR(nabto_device_add_tcp_tunnel_service_metadata(dev, "rtsp", "rtsp-path", "/mjpeg/1"));

    CHECK_NABTO_ERR(nabto_device_limit_connections(dev, 8));
    CHECK_NABTO_ERR(nabto_device_limit_stream_segments(dev, 200));

    NabtoDeviceFuture* future = nabto_device_future_new(dev);
    CHECK_NULL(future);

    nabto_device_start(dev, future);
    CHECK_NABTO_ERR(nabto_device_future_wait(future));
    nabto_device_future_free(future);

    char* fingerprint = NULL;
    CHECK_NABTO_ERR(nabto_device_get_device_fingerprint(dev, &fingerprint));

    ESP_LOGI(TAG, "Started nabto device with fingerprint %s", fingerprint);
    nabto_device_string_free(fingerprint);

    struct device_event_handler eh;
    device_event_handler_init(&eh, dev);

    struct connection_event_handler ceh;
    connection_event_handler_init(&ceh, dev);

    xTaskCreate(rtsp_server, "rtsp-server", 3584, NULL, tskIDLE_PRIORITY, &xHandle_RTSP_Server);

    perfmon_start();
    
    for (int i = 0;; i++) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        fflush(stdout);
    }

    esp_restart();
}
