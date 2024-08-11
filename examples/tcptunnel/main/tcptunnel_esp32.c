#include <nabto/nabto_device.h>
#include <modules/mbedtls/nm_mbedtls_spake2.h>
#include <core/nc_spake2.h>
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

#define CHECK_NABTO_ERR(err) do { if (err != NABTO_DEVICE_EC_OK) { ESP_LOGE(TAG, "Unexpected error at %s:%d, %s", __FILE__, __LINE__, nabto_device_error_get_message(err) ); esp_restart(); } } while (0)
#define CHECK_NULL(ptr) do { if (ptr == NULL) { ESP_LOGE(TAG, "Unexpected out of memory at %s:%d", __FILE__, __LINE__); esp_restart(); } } while (0)

struct use_all_memory_item {
    struct nn_llist_node node;
    uint8_t buffer[1024];
};

static uint32_t entropy_seed = 0xDEADBEEF;

// PRNG that we can seed with a deterministic value.
static uint32_t xorshift32(uint32_t* seed)
{
    uint32_t x = *seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *seed = x;
    return x;
}

static int xorshift_entropy_func(void *data, unsigned char *output, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        uint32_t x = xorshift32(&entropy_seed);
        output[i] = *(unsigned char*)&x;
    }
    return 0;
}

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

    //xTaskCreatePinnedToCore(stats_task, "stats", 4096, NULL, STATS_TASK_PRIO, NULL, tskNO_AFFINITY);

    {
        struct nc_spake2_password_request* req = nc_spake2_password_request_new();
        uint8_t clifp[] = {0xcf, 0xf2, 0xf6, 0x5c, 0xd1, 0x03, 0x48, 0x8b,
                            0x8c, 0xb2, 0xb9, 0x3e, 0x83, 0x8a, 0xcc, 0x0f,
                            0x71, 0x9d, 0x6d, 0xea, 0xe3, 0x7f, 0x8a, 0x4b,
                            0x74, 0xfa, 0x82, 0x52, 0x44, 0xd2, 0x8a, 0xf8};
        uint8_t devfp[] = {0x73, 0xe5, 0x30, 0x42, 0x55, 0x1c, 0x12, 0x8a,
                            0x49, 0x2c, 0xfd, 0x91, 0x0b, 0x9b, 0xa6, 0x7f,
                            0xff, 0xd2, 0xca, 0xb6, 0xc0, 0x23, 0xb5, 0x0c,
                            0x10, 0x99, 0x22, 0x89, 0xf4, 0xc2, 0x3d, 0x54};

        memcpy(req->clientFingerprint, clifp, sizeof(clifp));
        memcpy(req->deviceFingerprint, devfp, sizeof(clifp));
        uint8_t T[] = {
            0x04, 0xB3, 0x1A, 0x66, 0xC5, 0x28, 0x4A, 0x26,
            0x33, 0x5D, 0xF6, 0xDF, 0x17, 0x86, 0x91, 0xB8,
            0xE8, 0xE3, 0xFB, 0x41, 0x7E, 0xBB, 0xE4, 0xF7,
            0x2D, 0x50, 0xFC, 0x83, 0xAF, 0x12, 0x32, 0xE2,
            0x38, 0x8F, 0x1B, 0xA6, 0xD2, 0x09, 0xB6, 0x0C,
            0xC3, 0x1F, 0x57, 0x60, 0xF4, 0xD3, 0xC6, 0x8B,
            0xE2, 0xB4, 0x3D, 0x28, 0x11, 0x4B, 0x06, 0xC1,
            0xB4, 0xEE, 0x36, 0x5C, 0xC6, 0x5A, 0x69, 0xC7,
            0x98
        };

        req->T = T;
        req->Tlen = 65;

        uint8_t S[256];
        size_t SLen = sizeof(S);
        uint8_t key[32];

        nm_mbedtls_spake2_calculate_key(NULL, req, xorshift_entropy_func, "FFzeqrpJTVF4", S, &SLen, key);
        for (int i = 0; i < 32; i += 4)
        {
            ESP_LOGI(LOGM, "%02X %02X %02X %02X", key[i+0], key[i+1], key[i+2], key[i+3]);
        }
    }

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
