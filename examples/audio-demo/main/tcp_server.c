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



static const char *TAG = "TCP_SERVER_STREAM_EXAMPLE";

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

    CHECK_NABTO_ERR(nabto_device_limit_connections(dev, 3));
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

    ESP_LOGI(TAG, "Pairring string: p=%s,d=%s,pwd=%s,sct=%s", CONFIG_EXAMPLE_NABTO_PRODUCT_ID,CONFIG_EXAMPLE_NABTO_DEVICE_ID, "pairpsw", sct);

    
    struct device_event_handler eh;
    device_event_handler_init(&eh, dev);

    struct connection_event_handler ceh;
    connection_event_handler_init(&ceh, dev);

    
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_set("TCP_SERVER_STREAM", ESP_LOG_DEBUG);
    esp_log_level_set(LOGM, ESP_LOG_DEBUG);
    
    
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = CONFIG_TCP_SERVER_KEEPALIVE_IDLE;
    int keepInterval = CONFIG_TCP_SERVER_KEEPALIVE_INTERVAL;
    int keepCount = CONFIG_TCP_SERVER_KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;
    
    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        //dest_addr_ip4->sin_addr.s_addr = inet_addr("127.0.0.1");
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(8000);
        ip_protocol = IPPROTO_IP;
    }
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        goto _exit;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    
    ESP_LOGI(TAG, "Socket created");
    
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto _exit;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", 8000);

    
    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto _exit;
    }


    
    
    ESP_LOGI(TAG, "Socket listening");

    while(true) {
        
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            goto _exit;
        }
    
        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
        inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
    
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        
        /*
         * Setup audio pipeline based on accepted socket
         */
        audio_pipeline_handle_t read_pipeline, write_pipeline;
        audio_element_handle_t tcp_stream_reader, tcp_stream_writer, i2s_stream_writer, i2s_stream_reader;
        audio_element_handle_t wav_decoder;

        
        
        
        ESP_LOGI(TAG, "[ 1 ] Start codec chip");
        audio_board_handle_t board_handle = audio_board_init();
        audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);


        // Set volume
        audio_hal_set_volume(board_handle->audio_hal, 99); // Set volume  (range is 0-100)

        
        ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
        audio_pipeline_cfg_t read_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
        audio_pipeline_cfg_t write_pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
        read_pipeline = audio_pipeline_init(&read_pipeline_cfg);
        write_pipeline = audio_pipeline_init(&write_pipeline_cfg);
        AUDIO_NULL_CHECK(TAG, read_pipeline, return);
        AUDIO_NULL_CHECK(TAG, write_pipeline, return);

        // First the reader pipepline, ie. read from the socket and write to speaker
        
        ESP_LOGI(TAG, "[2.1] Create i2s stream to write data to codec chip");
        i2s_stream_cfg_t read_i2s_cfg = I2S_STREAM_CFG_DEFAULT();
        read_i2s_cfg.type = AUDIO_STREAM_WRITER;

        // Something we have tested:
        //i2s_stream_cfg_t read_i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(CODEC_ADC_I2S_PORT, 8000, 16, AUDIO_STREAM_WRITER);
        //read_i2s_cfg.std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
        //read_i2s_cfg.chan_cfg.dma_desc_num = 8;
        //read_i2s_cfg.chan_cfg.dma_frame_num = 1024; // Increase buffer length
        //read_i2s_cfg.out_rb_size = 32 * 1024; // Increase buffer to avoid missing data in bad network conditions
        
        i2s_stream_reader = i2s_stream_init(&read_i2s_cfg);
        AUDIO_NULL_CHECK(TAG, i2s_stream_reader, return);

        // 16000hz 16bit 2 channels
        i2s_stream_set_clk(i2s_stream_reader, 16000, 16, 2);

        //
        // Create upsample filter ie. 8000hz 16bit mono -> 16000hz 16bit stereo
        // raw i2s cannot handle mono and 8000 .. very poor document 
        //
        rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
        rsp_cfg.src_rate = 16000;
        rsp_cfg.src_ch = 1;
        rsp_cfg.dest_rate = 16000;
        rsp_cfg.dest_ch = 2;
        rsp_cfg.mode = RESAMPLE_DECODE_MODE;
        rsp_cfg.complexity = 0;
        audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);
        

        // Create wav decoder
        ESP_LOGI(TAG, "[2.2] Create wav decoder to decode wav file/stream");
        wav_decoder_cfg_t wav_decoder_cfg = DEFAULT_WAV_DECODER_CONFIG();
        wav_decoder_cfg.out_rb_size = 20*1024;
        
        wav_decoder = wav_decoder_init(&wav_decoder_cfg);
        AUDIO_NULL_CHECK(TAG, wav_decoder, return);

        // Create tcp endpoint
        ESP_LOGI(TAG, "[2.2] Create tcp server stream to read data from");
        tcp_server_stream_cfg_t read_tcp_cfg = TCP_SERVER_STREAM_CFG_DEFAULT();
        read_tcp_cfg.type = AUDIO_STREAM_READER;
        read_tcp_cfg.connect_sock = sock;

        
        tcp_stream_reader = tcp_server_stream_init(&read_tcp_cfg);
        AUDIO_NULL_CHECK(TAG, tcp_stream_reader, return);


        ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
        audio_pipeline_register(read_pipeline, tcp_stream_reader, "tcp");
        audio_pipeline_register(read_pipeline, wav_decoder, "wav_decode");
        audio_pipeline_register(read_pipeline, filter, "upsample");
        audio_pipeline_register(read_pipeline, i2s_stream_reader, "i2s");
        
        ESP_LOGI(TAG, "[2.4] Link it together tcp-->wav_decode-->upsample->i2s");
        //audio_pipeline_link(read_pipeline, (const char *[]) {"tcp", "wav_decode", "upsample", "i2s"}, 4);
        audio_pipeline_link(read_pipeline, (const char *[]) {"tcp", "upsample", "i2s"}, 3);

        
        // Second the writer pipepline, ie. read from the microphone and forward to socket
        
        ESP_LOGI(TAG, "[2.1b] Create i2s stream to read data to codec chip");
        i2s_stream_cfg_t write_i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(CODEC_ADC_I2S_PORT, 8000, 16, AUDIO_STREAM_READER);

        
        write_i2s_cfg.std_cfg.slot_cfg.slot_mode=1;
        write_i2s_cfg.std_cfg.slot_cfg.slot_mask=I2S_STD_SLOT_RIGHT;

        //i2s_stream_cfg_t write_i2s_cfg = I2S_STREAM_CFG_DEFAULT();
        
        write_i2s_cfg.volume=100;
        //write_i2s_cfg.out_rb_size = 32 * 1024; // Increase buffer to avoid missing data in bad network conditions

        
        write_i2s_cfg.type = AUDIO_STREAM_READER;
        i2s_stream_writer = i2s_stream_init(&write_i2s_cfg);

        AUDIO_NULL_CHECK(TAG, i2s_stream_writer, return);
        //i2s_stream_set_clk(i2s_stream_writer, 16000, 16, 1);


        //
        // Create downsample filter : 16000hz 16bit steroe -> 8000hz 16bit mono
        // raw i2s cannot handle mono and 8000 .. very poor documentation 
        //
        rsp_filter_cfg_t rsp_cfg_in = DEFAULT_RESAMPLE_FILTER_CONFIG();
        rsp_cfg_in.src_rate = 16000;
        rsp_cfg_in.src_ch = 2;
        rsp_cfg_in.dest_rate = 8000;
        rsp_cfg_in.dest_ch = 1;
        rsp_cfg_in.mode = RESAMPLE_DECODE_MODE;
        rsp_cfg_in.complexity = 1;
        audio_element_handle_t filter_in = rsp_filter_init(&rsp_cfg_in);

        
        ESP_LOGI(TAG, "[2.1c] Create tcp server stream to write data");
        tcp_server_stream_cfg_t write_tcp_cfg = TCP_SERVER_STREAM_CFG_DEFAULT();
        write_tcp_cfg.type = AUDIO_STREAM_WRITER;
        write_tcp_cfg.connect_sock = sock;
        tcp_stream_writer = tcp_server_stream_init(&write_tcp_cfg);
        AUDIO_NULL_CHECK(TAG, tcp_stream_writer, return);

        ESP_LOGI(TAG, "[2.2c] Register all elements to audio pipeline");
        audio_pipeline_register(write_pipeline, tcp_stream_writer, "tcp");
        audio_pipeline_register(write_pipeline, i2s_stream_writer, "i2s");
        audio_pipeline_register(write_pipeline, filter_in, "downsample");
        
        ESP_LOGI(TAG, "[2.3c] Link it together i2s-->downsample-->tcp");
        //audio_pipeline_link(write_pipeline, (const char *[]) {"i2s", "downsample", "tcp"}, 3);
        audio_pipeline_link(write_pipeline, (const char *[]) {"i2s", "tcp"}, 2);
                
        ESP_LOGI(TAG, "[ 3 ] Start pipelines");


        // Start the pipelines
        audio_pipeline_run(write_pipeline);
        audio_pipeline_run(read_pipeline);


        

        ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
        
        ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
        audio_pipeline_set_listener(read_pipeline, evt);
        audio_pipeline_set_listener(write_pipeline, evt);
        
        

        // Wait on a close
        while(true) {
            audio_event_iface_msg_t msg;
            esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
                continue;
            }
            
            /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s_stream_writer
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
                && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
                ESP_LOGW(TAG, "[ * ] Stop event received");
                break;
            }
            if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) tcp_stream_writer
                && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) {
                ESP_LOGW(TAG, "[ * ] Event received from tcp_stream_writer %d", (int)msg.data);
                break;
                
            }
            //   && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))) {
            
            
            
        }


        ESP_LOGI(TAG, "[ 6.1 ] Stop read audio_pipeline");
        audio_pipeline_stop(read_pipeline);
        audio_pipeline_wait_for_stop(read_pipeline);
        audio_pipeline_terminate(read_pipeline);

        ESP_LOGI(TAG, "[ 6.2 ] Stop write audio_pipeline");
        audio_pipeline_stop(write_pipeline);
        audio_pipeline_wait_for_stop(write_pipeline);
        audio_pipeline_terminate(write_pipeline);
        
        /* Terminate the pipeline before removing the listener */

        audio_pipeline_unregister(read_pipeline, tcp_stream_reader);
        //audio_pipeline_unregister(read_pipeline, i2s_stream_reader);
        //audio_pipeline_unregister(read_pipeline, wav_decoder);
        
        audio_pipeline_unregister(write_pipeline, tcp_stream_writer);
        audio_pipeline_unregister(write_pipeline, i2s_stream_writer);
        //audio_pipeline_unregister(write_pipeline, wav_encoder);
        
        
        //audio_pipeline_remove_listener(read_pipeline);
        audio_pipeline_remove_listener(write_pipeline);


        
    }

  _exit:
    close(listen_sock);


}
