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
#include "protocol_examples_common.h"

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
#include "i2s_stream.h"
#include "driver/i2s.h"

#include "board.h"
#include "audio_idf_version.h"

// This component setup
#include "tcp_server_stream.h"


/*
#define PORT                        CONFIG_EXAMPLE_PORT
#define KEEPALIVE_IDLE              CONFIG_EXAMPLE_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_EXAMPLE_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_EXAMPLE_KEEPALIVE_COUNT
*/

static const char *TAG = "TCP_SERVER_STREAM_EXAMPLE";


// I2S pin configuration for LyraT Mini
#define I2S_BCK_PIN    27
#define I2S_WS_PIN     26
#define I2S_DO_PIN     25
#define I2S_DI_PIN     35

void setup_i2s_pins() {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        //.communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DO_PIN,
        .data_in_num = I2S_DI_PIN
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}



/**
 * Hagroy example
 */
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);


    //setup_i2s_pins();

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
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(CONFIG_TCP_SERVER_PORT);
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
    ESP_LOGI(TAG, "Socket bound, port %d", CONFIG_TCP_SERVER_PORT);

    //_dispatch_event(self, tcp, NULL, 0, TCP_SERVER_STREAM_STATE_CONNECTED);
    
    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto _exit;
    }



    ESP_LOGI(TAG, "Socket listening");

    while(1) {
    
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
        //audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);
        audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

        // Set volume
        audio_hal_set_volume(board_handle->audio_hal, 50); // Set volume to 50% (range is 0-100)

        
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
        i2s_stream_reader = i2s_stream_init(&read_i2s_cfg);
        ESP_LOGI(TAG, "her1");
        AUDIO_NULL_CHECK(TAG, i2s_stream_reader, return);
        ESP_LOGI(TAG, "her2");
        i2s_stream_set_clk(i2s_stream_reader, 16000, 16, 2);
        ESP_LOGI(TAG, "her3");

        
        ESP_LOGI(TAG, "[2.2] Create wav decoder to decode wav file/stream");
        wav_decoder_cfg_t wav_decoder_cfg = DEFAULT_WAV_DECODER_CONFIG();
        wav_decoder = wav_decoder_init(&wav_decoder_cfg);
        AUDIO_NULL_CHECK(TAG, wav_decoder, return);

        ESP_LOGI(TAG, "[2.2] Create tcp server stream to write data");
        tcp_server_stream_cfg_t read_tcp_cfg = TCP_SERVER_STREAM_CFG_DEFAULT();
        read_tcp_cfg.type = AUDIO_STREAM_READER;
        read_tcp_cfg.connect_sock = sock;
        tcp_stream_reader = tcp_server_stream_init(&read_tcp_cfg);
        AUDIO_NULL_CHECK(TAG, tcp_stream_reader, return);


        ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
        audio_pipeline_register(read_pipeline, tcp_stream_reader, "tcp");
        audio_pipeline_register(read_pipeline, wav_decoder, "wav_decode");
        audio_pipeline_register(read_pipeline, i2s_stream_reader, "i2s");
        
        ESP_LOGI(TAG, "[2.4] Link it together tcp-->wav_decode-->i2s");
        audio_pipeline_link(read_pipeline, (const char *[]) {"tcp", "wav_decode", "i2s"}, 3);
        
        
        // Second the writer pipepline, ie. read from the microphone and forward to socket
        
        ESP_LOGI(TAG, "[2.1b] Create i2s stream to read data to codec chip");
        //i2s_stream_cfg_t write_i2s_cfg = I2S_STREAM_CFG_DEFAULT();
        i2s_stream_cfg_t write_i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(CODEC_ADC_I2S_PORT, 16000, 16, AUDIO_STREAM_READER);
        write_i2s_cfg.i2s_config.mode=(i2s_mode_t)(I2S_MODE_MASTER |  I2S_MODE_RX);
        
        write_i2s_cfg.out_rb_size = 16 * 1024; // Increase buffer to avoid missing data in bad network conditions

        
        write_i2s_cfg.type = AUDIO_STREAM_READER;
        i2s_stream_writer = i2s_stream_init(&write_i2s_cfg);
        ESP_LOGI(TAG, "her1");
        AUDIO_NULL_CHECK(TAG, i2s_stream_writer, return);
        i2s_stream_set_clk(i2s_stream_writer, 16000, 16, 2);

        
        ESP_LOGI(TAG, "[2.1c] Create tcp server stream to write data");
        tcp_server_stream_cfg_t write_tcp_cfg = TCP_SERVER_STREAM_CFG_DEFAULT();
        write_tcp_cfg.type = AUDIO_STREAM_WRITER;
        write_tcp_cfg.connect_sock = sock;
        tcp_stream_writer = tcp_server_stream_init(&write_tcp_cfg);
        AUDIO_NULL_CHECK(TAG, tcp_stream_writer, return);

        ESP_LOGI(TAG, "[2.2c] Register all elements to audio pipeline");
        audio_pipeline_register(write_pipeline, tcp_stream_writer, "tcp");
        audio_pipeline_register(write_pipeline, i2s_stream_writer, "i2s");
        
        ESP_LOGI(TAG, "[2.3c] Link it together i2s-->wav_encode-->tcp");
        audio_pipeline_link(write_pipeline, (const char *[]) {"i2s", "tcp"}, 2);
                
        ESP_LOGI(TAG, "[ 3 ] Start pipelines");


        // Start the pipelines
        audio_pipeline_run(write_pipeline);
        audio_pipeline_run(read_pipeline);


        

        ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
        audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
        audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
        
        ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
        //audio_pipeline_set_listener(read_pipeline, evt);
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

        
    
    //xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);

}
