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



static const char *TAG = "AUDIO_SERVER";

/*
 * The audio server
 * Basicly just listen in on incomming connections
 * Once an incomming connection is accepted it is used to set up two audio pipelines
 * one pipeline is reading from the socket and pushing the data to the audio I2S out
 * another pipeline is reading from I2S and pushing the data to the socket 
 */
void audio_server(void*) {

    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    int listen_sock;
    struct sockaddr_in servaddr, cliaddr;

    
    // Creating socket file descriptor
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ESP_LOGE(TAG, "socket creation failure");
        return;
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    
    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8000);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind the socket to the port
    if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        ESP_LOGE(TAG, "bind failure");
        close(listen_sock);
        goto _exit;
    }
    // Listen for incoming connections
    if (listen(listen_sock, 5) < 0) {
        ESP_LOGE(TAG, "listen failure");
        close(listen_sock);
            goto _exit;
    }

    
    ESP_LOGI(TAG, "Socket created");
    
    while(true) {

        struct sockaddr_in source_addr;
        socklen_t slen;
            

        ESP_LOGI(TAG, "Waiting for incomming sockets");

        //while(true) { vTaskDelay(10000 / portTICK_PERIOD_MS);}

        
        memset(&source_addr, 0, sizeof(source_addr));
        slen = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &slen);


        
        ESP_LOGI(TAG, "Accepted socket");


        
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            goto _exit;
        }
    

        
        /*
         * Setup audio pipeline based on accepted socket
         */
        audio_pipeline_handle_t read_pipeline, write_pipeline;
        audio_element_handle_t tcp_stream_reader, tcp_stream_writer, i2s_stream_writer, i2s_stream_reader;

        
        
        
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

        i2s_stream_reader = i2s_stream_init(&read_i2s_cfg);
        AUDIO_NULL_CHECK(TAG, i2s_stream_reader, return);

        // 16000hz 16bit 2 channels
        i2s_stream_set_clk(i2s_stream_reader, 16000, 16, 2);

        //
        // Create upsample filter ie. 8000hz 16bit mono -> 16000hz 16bit stereo
        // raw i2s cannot handle mono and 8000 .. very poor document 
        //
        rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
        rsp_cfg.src_rate = 8000;
        rsp_cfg.src_ch = 1;
        rsp_cfg.dest_rate = 16000;
        rsp_cfg.dest_ch = 2;
        rsp_cfg.mode = RESAMPLE_DECODE_MODE;
        rsp_cfg.complexity = 0;
        audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);
        

        // Create tcp endpoint
        ESP_LOGI(TAG, "[2.2] Create tcp server stream to read data from");
        tcp_server_stream_cfg_t read_tcp_cfg = TCP_SERVER_STREAM_CFG_DEFAULT();
        read_tcp_cfg.type = AUDIO_STREAM_READER;
        read_tcp_cfg.connect_sock = sock;

        
        tcp_stream_reader = tcp_server_stream_init(&read_tcp_cfg);
        AUDIO_NULL_CHECK(TAG, tcp_stream_reader, return);


        ESP_LOGI(TAG, "[2.3] Register all elements to audio pipeline");
        audio_pipeline_register(read_pipeline, tcp_stream_reader, "tcp");
        audio_pipeline_register(read_pipeline, filter, "upsample");
        audio_pipeline_register(read_pipeline, i2s_stream_reader, "i2s");
        
        ESP_LOGI(TAG, "[2.4] Link it together tcp-->upsample->i2s");
        audio_pipeline_link(read_pipeline, (const char *[]) {"tcp", "upsample", "i2s"}, 3);

        
        // Second the writer pipepline, ie. read from the microphone and forward to socket
        
        ESP_LOGI(TAG, "[2.1b] Create i2s stream to read data to codec chip");
        i2s_stream_cfg_t write_i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(CODEC_ADC_I2S_PORT, 8000, 16, AUDIO_STREAM_READER);

        
        write_i2s_cfg.std_cfg.slot_cfg.slot_mode=1;
        write_i2s_cfg.std_cfg.slot_cfg.slot_mask=I2S_STD_SLOT_RIGHT;

        write_i2s_cfg.volume=100;
        
        write_i2s_cfg.type = AUDIO_STREAM_READER;
        i2s_stream_writer = i2s_stream_init(&write_i2s_cfg);

        AUDIO_NULL_CHECK(TAG, i2s_stream_writer, return);

        
        ESP_LOGI(TAG, "[2.1c] Create tcp server stream to write data");
        tcp_server_stream_cfg_t write_tcp_cfg = TCP_SERVER_STREAM_CFG_DEFAULT();
        write_tcp_cfg.type = AUDIO_STREAM_WRITER;
        write_tcp_cfg.connect_sock = sock;
        tcp_stream_writer = tcp_server_stream_init(&write_tcp_cfg);
        AUDIO_NULL_CHECK(TAG, tcp_stream_writer, return);

        ESP_LOGI(TAG, "[2.2c] Register all elements to audio pipeline");
        audio_pipeline_register(write_pipeline, tcp_stream_writer, "tcp");
        audio_pipeline_register(write_pipeline, i2s_stream_writer, "i2s");
        
        ESP_LOGI(TAG, "[2.3c] Link it together i2s-->tcp");
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
        audio_pipeline_unregister(read_pipeline, filter);
        audio_pipeline_unregister(read_pipeline, i2s_stream_reader);

        audio_pipeline_unregister(write_pipeline, tcp_stream_writer);
        audio_pipeline_unregister(write_pipeline, i2s_stream_writer);
        
        audio_pipeline_remove_listener(write_pipeline);
        
    }

  _exit:
    close(listen_sock);


}
