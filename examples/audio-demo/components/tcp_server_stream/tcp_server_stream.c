/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "lwip/sockets.h"
#include "esp_log.h"
#include "esp_err.h"
#include "audio_mem.h"
#include "tcp_server_stream.h"

static const char *TAG = "TCP_SERVER_STREAM";
#define CONNECT_TIMEOUT_MS        1000

typedef struct tcp_server_stream {
    audio_stream_type_t           type;
    int                           connect_sock;
    bool                          is_open;
    int                           timeout_ms;
    tcp_server_stream_event_handle_cb    hook;
    void                          *ctx;
} tcp_server_stream_t;


#define KEEPALIVE_IDLE              CONFIG_TCP_SERVER_KEEPALIVE_IDLE
#define KEEPALIVE_INTERVAL          CONFIG_TCP_SERVER_KEEPALIVE_INTERVAL
#define KEEPALIVE_COUNT             CONFIG_TCP_SERVER_KEEPALIVE_COUNT


/*
 * Helper function
 */
static int _get_socket_error_code_reason(const char *str, int sockfd)
{
    uint32_t optlen = sizeof(int);
    int result;
    int err;

    err = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &result, &optlen);
    if (err == -1) {
        ESP_LOGE(TAG, "%s, getsockopt failed (%d)", str, err);
        return -1;
    }
    if (result != 0) {
        ESP_LOGW(TAG, "%s error, error code: %d, reason: %s", str, err, strerror(result));
    }
    return result;
}

static esp_err_t _dispatch_event(audio_element_handle_t el, tcp_server_stream_t *tcp, void *data, int len, tcp_server_stream_status_t state)
{

    ESP_LOGD(TAG, "tcp_server_stream : dispatch_event()");
    
    if (el && tcp && tcp->hook) {
        tcp_server_stream_event_msg_t msg = { 0 };
        msg.data = data;
        msg.data_len = len;
//        msg.sock_fd = tcp->t;
        msg.source = el;
        return tcp->hook(&msg, state, tcp->ctx);
    }
    return ESP_FAIL;
}

static esp_err_t _tcp_server_open(audio_element_handle_t self)
{
    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);
    
    ESP_LOGD(TAG, "tcp_server_stream_open()");
    
    tcp_server_stream_t *tcp = (tcp_server_stream_t *)audio_element_getdata(self);

    
    _dispatch_event(self, tcp, NULL, 0, TCP_SERVER_STREAM_STATE_CONNECTED);
    
    return ESP_OK;
    
    
}

static esp_err_t _tcp_server_read(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
    tcp_server_stream_t *tcp = (tcp_server_stream_t *)audio_element_getdata(self);

    ESP_LOGD(TAG, "tcp_server_stream_read()");

    
    int rlen = read(tcp->connect_sock, buffer, len);
    if (rlen < 0) {
        if (rlen == -1) {
            ESP_LOGW(TAG, "TCP server actively closes the connection");
            return ESP_OK;
        } else {
            ESP_LOGE(TAG, "read data failed");
            _get_socket_error_code_reason(__func__, tcp->connect_sock);
            _dispatch_event(self, tcp, NULL, 0, TCP_SERVER_STREAM_STATE_ERROR);
            return ESP_FAIL;
        }
    }

    audio_element_update_byte_pos(self, rlen);
    ESP_LOGD(TAG, "read len=%d, rlen=%d", len, rlen);
    return rlen;
}

static esp_err_t _tcp_server_write(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{

    ESP_LOGD(TAG, "tcp_server_stream_write()");
    tcp_server_stream_t *tcp = (tcp_server_stream_t *)audio_element_getdata(self);
    int wlen = 0;
    int sum = 0;
    for(int i=0;i<len;i++) {
        sum += buffer[i];
    }
    printf("socket:%d . buffer sum: %d \n", tcp->connect_sock, sum);
    ESP_LOGD(TAG, "socket:%d . buffer sum: %d ", tcp->connect_sock, sum);
    
    wlen = write(tcp->connect_sock, buffer, len);
    if (wlen < 0) {
        ESP_LOGE(TAG, "write data failed:%d",wlen);
        _get_socket_error_code_reason(__func__, tcp->connect_sock);
        _dispatch_event(self, tcp, NULL, 0, TCP_SERVER_STREAM_STATE_ERROR);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "write len=%d", len);
    return wlen;
}

static esp_err_t _tcp_server_process(audio_element_handle_t self, char *in_buffer, int in_len)
{

    ESP_LOGD(TAG, "tcp_server_stream_process()");
    
    int r_size = audio_element_input(self, in_buffer, in_len);
    int w_size = 0;
    if (r_size > 0) {
        w_size = audio_element_output(self, in_buffer, r_size);
        if (w_size > 0) {
            audio_element_update_byte_pos(self, r_size);
        }
    } else {
        w_size = r_size;
    }
    return w_size;
}

static esp_err_t _tcp_server_close(audio_element_handle_t self)
{
    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);

    ESP_LOGD(TAG, "tcp_server_stream_close()");
    
    tcp_server_stream_t *tcp = (tcp_server_stream_t *)audio_element_getdata(self);
    tcp=tcp;
    AUDIO_NULL_CHECK(TAG, tcp, return ESP_FAIL);
    close(tcp->connect_sock);

    _dispatch_event(self, tcp, NULL, 0, TCP_SERVER_STREAM_STATE_CLOSE);
    
    return ESP_OK;
}

static esp_err_t _tcp_server_destroy(audio_element_handle_t self)
{
    AUDIO_NULL_CHECK(TAG, self, return ESP_FAIL);

    ESP_LOGD(TAG, "tcp_server_stream_destroy()");
    
    tcp_server_stream_t *tcp = (tcp_server_stream_t *)audio_element_getdata(self);
    
    AUDIO_NULL_CHECK(TAG, tcp, return ESP_FAIL);

    /*
    if (tcp->transport_list) {
        esp_transport_list_destroy(tcp->transport_list);
    } 
    */
    audio_free(tcp);
    return ESP_OK;
}

audio_element_handle_t tcp_server_stream_init(tcp_server_stream_cfg_t *config)
{
    // Comment out if you don't want to debug
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    AUDIO_NULL_CHECK(TAG, config, return NULL);

    ESP_LOGD(TAG, "tcp_server_stream_init()");
    
    audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
    audio_element_handle_t el;
    cfg.open = _tcp_server_open;
    cfg.close = _tcp_server_close;
    cfg.process = _tcp_server_process;
    cfg.destroy = _tcp_server_destroy;
    cfg.task_stack = config->task_stack;
    cfg.task_prio = config->task_prio;
    cfg.task_core = config->task_core;
    cfg.stack_in_ext = config->ext_stack;
    cfg.tag = "tcp_server";
    if (cfg.buffer_len == 0) {
        cfg.buffer_len = TCP_SERVER_STREAM_BUF_SIZE;
    }

    
    tcp_server_stream_t *tcp = audio_calloc(1, sizeof(tcp_server_stream_t));
    AUDIO_MEM_CHECK(TAG, tcp, return NULL);

    tcp->connect_sock = config->connect_sock;
    tcp->timeout_ms = config->timeout_ms;
    if (config->event_handler) {
        tcp->hook = config->event_handler;
        if (config->event_ctx) {
            tcp->ctx = config->event_ctx;
        }
    }

    if (config->type == AUDIO_STREAM_WRITER) {
        cfg.write = _tcp_server_write;
        ESP_LOGD(TAG, "tcp_server_stream set to writer");
    } else {
        ESP_LOGD(TAG, "tcp_server_stream set to reader");
        cfg.read = _tcp_server_read;
    }

    el = audio_element_init(&cfg);
    audio_element_set_output_ringbuf_size(el, 20*1024);
        
    AUDIO_MEM_CHECK(TAG, el, goto _tcp_init_exit);
    audio_element_setdata(el, tcp);

    return el;
_tcp_init_exit:

    ESP_LOGD(TAG, "tcp_server_stream_init() could not init");
    audio_free(tcp);
    return NULL;
}
