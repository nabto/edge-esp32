
#include "simple_perf.h"

#include "esp_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"



//static struct nn_log* perf_logger;
static const char* LOGM = "perf_server";

#define READSIZE 1024
#define PORT 9000

// 20 x 50 chars line = 1000 bytes
static const char* resp =
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n"           \
        "0123456789012345678901234567890123456789012345678\n";


/*
 * Function for starting the performance server
 */
void perf_task(void *pvParameter) {

    int listenfd, conn_sock, n;
    char buffer[READSIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;

    // Creating socket file descriptor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ESP_LOGE(LOGM, "socket creation failure");
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    // Bind the socket to the port
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        ESP_LOGE(LOGM, "bind failure");
        close(listenfd);
        return;
    }
    // Listen for incoming connections
    if (listen(listenfd, 5) < 0) {
        ESP_LOGE(LOGM, "listen failure");
        close(listenfd);
            return;
    }

    while(true) {

        if ((conn_sock = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
            ESP_LOGE(LOGM, "accept failure");
            return;
        }

        ESP_LOGI(LOGM, "Socket accepted");

        fd_set readfds;
        fd_set writefds;
        struct timeval tv;

        int read_bytes, write_bytes, ticks_res, ticks_from_last;

        read_bytes=0;
        write_bytes=0;
        ticks_from_last = xTaskGetTickCount();
        ticks_res = 5000 / portTICK_PERIOD_MS; // Log every 5 seconds
        ESP_LOGI(LOGM, "Tick resolution:%d", ticks_res);

        while(true) {

            FD_ZERO(&readfds);
            FD_ZERO(&writefds);
            FD_SET(conn_sock, &readfds);
            FD_SET(conn_sock, &writefds);

            tv.tv_sec = 0;
            tv.tv_usec = 500; // 5 milliseconds timeout

            int activity = select(conn_sock + 1, &readfds, &writefds, NULL, &tv);
            if (activity < 0) {
                ESP_LOGE(LOGM, "select error");
                return;

            } else if (activity == 0) {
                // Timeout occurred
            }

            if (FD_ISSET(conn_sock, &readfds)) {
                n = 0;
                n = read(conn_sock, buffer, READSIZE);
                //ESP_LOGI(LOGM, "read:%d",n);
                if(n < 0) {
                    ESP_LOGI(LOGM, "socket closed");
                    goto _socket_close;
                }
                read_bytes += n;
            }
            if (FD_ISSET(conn_sock, &writefds)) {
                n=0;
                n = write(conn_sock, resp, 1000);
                //ESP_LOGI(LOGM, "write:%d",n);
                if(n >0)
                    write_bytes +=n;
                if(n < 0) {
                    ESP_LOGI(LOGM, "socket closed");
                    goto _socket_close;
                }
            }

            int ticks_gone = xTaskGetTickCount() - ticks_from_last;
            if(ticks_gone >= ticks_res) {

                ESP_LOGI(LOGM, "write bytes:%d, bandwidth:%0.1f bytes/sec", write_bytes, ((float)write_bytes)*1000 / pdTICKS_TO_MS(ticks_gone));
                ESP_LOGI(LOGM, "read bytes:%d, bandwidth:%.1f bytes/sec", read_bytes, ((float)read_bytes)*1000 / pdTICKS_TO_MS(ticks_gone));
                write_bytes=0;
                read_bytes=0;
                ticks_from_last = xTaskGetTickCount();
            }


        }

      _socket_close:
        ESP_LOGE(LOGM, "Close socket");
        close(conn_sock);

    }


}

