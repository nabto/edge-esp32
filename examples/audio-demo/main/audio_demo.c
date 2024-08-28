/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
//#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// ADF includes
#include "audio_hal.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "tcp_client_stream.h"
#include "wav_decoder.h"
#include "wav_encoder.h"
#include "pcm_decoder.h"
#include "i2s_stream.h"
#include "filter_resample.h"

//#include "driver/i2s.h"
#include "driver/i2s_std.h"


#include "board.h"
#include "audio_idf_version.h"

// This component setup
#include "tcp_server_stream.h"

// Nabto stuff
#include <nabto/nabto_device.h>
#include "nabto_esp32_example.h"
#include "nabto_esp32_iam.h"
#include "edgecam_iam.h"
#include "device_event_handler.h"
#include "connection_event_handler.h"
//#include "esp32_perfmon.h"
#include "nabto_esp32_util.h"

#include "audio_server.h"

static const char *TAG = "AUDIO_DEMO";

const char* sct = "demosct";



#define CHECK_NABTO_ERR(err) do { if (err != NABTO_DEVICE_EC_OK) { ESP_LOGE(LOGM, "Unexpected error at %s:%d, %s", __FILE__, __LINE__, nabto_device_error_get_message(err) ); printf("ERROR!!!\n"); esp_restart(); } } while (0)
#define CHECK_NULL(ptr) do { if (ptr == NULL) { ESP_LOGE(LOGM, "Unexpected out of memory at %s:%d", __FILE__, __LINE__); printf("MEMORY-ERROR\n"); esp_restart(); } } while (0)


static const char* LOGM = "nabto";
void logCallback(NabtoDeviceLogMessage* msg, void* data)
{
    //ESP_LOGI(LOGM, "%s", msg->message);
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





/**
 * Audio demo example
 */
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
    CHECK_NABTO_ERR(nabto_device_mdns_add_txt_item(dev, "fn", "tcptunnel"));

    CHECK_NABTO_ERR(nabto_device_set_app_name(dev, "Tcp Tunnel"));

    CHECK_NABTO_ERR(nabto_device_add_server_connect_token(dev, sct));
    
    struct nn_log logger;
    nabto_esp32_util_nn_log_init(&logger);

    struct nabto_esp32_iam iam;

    struct nm_iam_state* defaultIamState = tcptunnel_create_default_iam_state(dev);
    struct nm_iam_configuration* iamConfig = tcptunnel_create_iam_config();

    nabto_esp32_iam_init(&iam, dev, iamConfig, defaultIamState, nvsHandle);

    CHECK_NABTO_ERR(nabto_device_add_tcp_tunnel_service(dev, "audio-pcm", "audio-pcm", "127.0.0.1", 8000));

    CHECK_NABTO_ERR(nabto_device_limit_connections(dev, 5));
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

    ESP_LOGI(TAG, "Pairring string: p=%s,d=%s,pwd=%s,sct=%s", CONFIG_EXAMPLE_NABTO_PRODUCT_ID,CONFIG_EXAMPLE_NABTO_DEVICE_ID, CONFIG_EXAMPLE_PAIRPASS, sct);

    
    struct device_event_handler eh;
    device_event_handler_init(&eh, dev);

    struct connection_event_handler ceh;
    connection_event_handler_init(&ceh, dev);

    
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_set("TCP_SERVER_STREAM", ESP_LOG_DEBUG);
    esp_log_level_set(LOGM, ESP_LOG_DEBUG);

    TaskHandle_t xHandle_Audio_Server = NULL;

    xTaskCreate(audio_server, "audio-server", 16384, NULL, tskIDLE_PRIORITY, &xHandle_Audio_Server);

    
    for (int i = 0;; i++) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        heap_caps_print_heap_info(MALLOC_CAP_8BIT);
        fflush(stdout);
    }
    

}
