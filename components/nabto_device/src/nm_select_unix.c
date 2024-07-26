#include "nm_select_unix.h"
#include "nm_select_unix_udp.h"
#include "nm_select_unix_tcp.h"

#include <platform/np_logging.h>
#include <platform/np_util.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "esp_pthread.h"

#define LOG NABTO_LOG_MODULE_UDP

/**
 * Helper function declarations
 */
void nm_select_unix_build_fd_sets();
static void* network_thread(void* data);

static int nm_select_unix_inf_wait(struct nm_select_unix* ctx);

static void nm_select_unix_read(struct nm_select_unix* ctx, int nfds);

/**
 * Api functions start
 */
np_error_code nm_select_unix_init(struct nm_select_unix* ctx)
{
    nn_llist_init(&ctx->udpSockets);
    nn_llist_init(&ctx->tcpSockets);
    ctx->stopped = false;
    ctx->thread = 0;

    if (!nm_select_unix_notify_init(ctx)) {
        return NABTO_EC_FAILED;
    }

    return NABTO_EC_OK;
}

void nm_select_unix_deinit(struct nm_select_unix* ctx)
{
    nm_select_unix_stop(ctx);

    if (ctx->thread != 0) {
        pthread_join(ctx->thread, NULL);
    }
    nm_select_unix_notify_deinit(ctx);
}

static size_t NETWORK_THREAD_STACK_SIZE = CONFIG_NABTO_DEVICE_NETWORK_THREAD_STACK_SIZE;

void nm_select_unix_run(struct nm_select_unix* ctx)
{
    esp_pthread_cfg_t pthreadConfig = esp_pthread_get_default_config();
    pthreadConfig.stack_size = NETWORK_THREAD_STACK_SIZE;
    pthreadConfig.thread_name = "Nabto Network";
    pthreadConfig.prio = 15;
    //pthreadConfig.pin_to_core = 1;

    esp_err_t err = esp_pthread_set_cfg(&pthreadConfig);
    if (err != ESP_OK) {
        NABTO_LOG_ERROR(LOG, "Cannot set pthread cfg");
    }
    pthread_create(&ctx->thread, NULL, &network_thread, ctx);
}

void nm_select_unix_stop(struct nm_select_unix* ctx)
{
    if (ctx->stopped) {
        return;
    }
    ctx->stopped = true;
    nm_select_unix_notify(ctx);
}


int nm_select_unix_inf_wait(struct nm_select_unix* ctx)
{
    int nfds;
    nm_select_unix_build_fd_sets(ctx);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    nfds = select(NP_MAX(ctx->maxReadFd, ctx->maxWriteFd)+1, &ctx->readFds, &ctx->writeFds, NULL, &timeout);
    if (nfds < 0) {
        NABTO_LOG_ERROR(LOG, "Error in select: (%i) '%s'", errno, strerror(errno));
    } else {
        //NABTO_LOG_TRACE(LOG, "select returned with %i file descriptors", nfds);
    }
    return nfds;
}

int nm_select_unix_timed_wait(struct nm_select_unix* ctx, uint32_t ms)
{
    NABTO_LOG_TRACE(LOG, "timed wait %d", ms);
    int nfds;
    struct timeval timeout_val;
    timeout_val.tv_sec = (ms/1000);
    timeout_val.tv_usec = ((ms)%1000)*1000;
    nm_select_unix_build_fd_sets(ctx);
    nfds = select(NP_MAX(ctx->maxReadFd, ctx->maxWriteFd)+1, &ctx->readFds, &ctx->writeFds, NULL, &timeout_val);
    if (nfds < 0) {
        NABTO_LOG_ERROR(LOG, "Error in select wait: (%i) '%s'", errno, strerror(errno));
    }
    return nfds;
}

void nm_select_unix_read(struct nm_select_unix* ctx, int nfds)
{
    //NABTO_LOG_TRACE(LOG, "read: %i", nfds);
    if (FD_ISSET(ctx->notifyRecvSocket, &ctx->readFds)) {
        nm_select_unix_notify_read_from_socket(ctx);
    }

    nm_select_unix_udp_handle_select(ctx, nfds);
    nm_select_unix_tcp_handle_select(ctx, nfds);
}

void nm_select_unix_close(struct nm_select_unix* ctx)
{
}

/**
 * Helper functions start
 */

void nm_select_unix_build_fd_sets(struct nm_select_unix* ctx)
{
    FD_ZERO(&ctx->readFds);
    FD_ZERO(&ctx->writeFds);
    ctx->maxReadFd = 0;
    ctx->maxWriteFd = 0;

    nm_select_unix_udp_build_fd_sets(ctx);

    nm_select_unix_tcp_build_fd_sets(ctx);

    FD_SET(ctx->notifyRecvSocket, &ctx->readFds);
    ctx->maxReadFd = NP_MAX(ctx->maxReadFd, ctx->notifyRecvSocket);
}

bool nm_select_unix_notify_init(struct nm_select_unix* ctx) {
    ctx->notifyPort = 7890;
    if (nm_select_unix_notify_alloc_sockets(ctx) &&
        nm_select_unix_notify_bind_recv_socket(ctx)) {
        return true;
    } else {
        nm_select_unix_notify_deinit(ctx);
        return false;
    }
}

void nm_select_unix_notify_deinit(struct nm_select_unix* ctx) {
    close(ctx->notifyRecvSocket);
    close(ctx->notifySendSocket);
}

bool nm_select_unix_notify_alloc_sockets(struct nm_select_unix* ctx)
{
    ctx->notifyRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ctx->notifySendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (ctx->notifyRecvSocket == -1 || ctx->notifySendSocket == -1) {
        return false;
    }
    //if (!nm_select_unix_notify_set_nonblocking(ctx->notifyRecvSocket) || !nm_select_unix_notify_set_nonblocking(ctx->notifySendSocket)) {
    //    return false;
    //}
    return true;
}
bool nm_select_unix_notify_set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        NABTO_LOG_ERROR(LOG, "cannot set nonblocking mode, fcntl F_GETFL failed");
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        NABTO_LOG_ERROR(LOG, "cannot set nonblocking mode, fcntl F_SETFL failed");
        return false;
    }
    return true;
}

bool nm_select_unix_notify_bind_recv_socket(struct nm_select_unix* ctx)
{
    struct sockaddr_storage addr = {};
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(ctx->notifyPort);
    inet_aton("127.0.0.1", &addr4->sin_addr);
    int status = bind(ctx->notifyRecvSocket, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0) {
        return false;
    }

    return true;
}

void nm_select_unix_notify_read_from_socket(struct nm_select_unix* ctx) {
    const size_t dataLength = 10;
    uint8_t data[dataLength];
    int status = recvfrom(ctx->notifyRecvSocket, data, dataLength, 0, NULL, NULL);
    if (status < 0) {
        NABTO_LOG_ERROR(LOG, "nm_select_unix_read_from_notify_socket error: (%i) '%s'", errno, strerror(errno));
    }
}

void nm_select_unix_notify(struct nm_select_unix* ctx)
{
    uint8_t byte;
    struct sockaddr_storage to_addr = {};
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&to_addr;
    addr4->sin_family = AF_INET;
    addr4->sin_port = htons(ctx->notifyPort);
    inet_aton("127.0.0.1", &addr4->sin_addr);
    int status = sendto(ctx->notifySendSocket, &byte, 1, 0,  (struct sockaddr *)&to_addr, sizeof(to_addr));
    if (status < 0) {
        NABTO_LOG_ERROR(LOG, "nm_select_unix_notify error: (%i) '%s'", errno, strerror(errno));
    }
}

void* network_thread(void* data)
{
    struct nm_select_unix* ctx = data;
    while(true) {
        int nfds;

        if (ctx->stopped) {
            return NULL;
        } else {
            // Wait for events.
            nfds = nm_select_unix_inf_wait(ctx);
            nm_select_unix_read(ctx, nfds);
        }
    }
    return NULL;
}
