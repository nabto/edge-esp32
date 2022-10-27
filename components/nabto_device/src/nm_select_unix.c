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

    return NABTO_EC_OK;
}

void nm_select_unix_deinit(struct nm_select_unix* ctx)
{
    nm_select_unix_stop(ctx);

    if (ctx->thread != 0) {
        pthread_join(ctx->thread, NULL);
    }
}

static size_t NETWORK_THREAD_STACK_SIZE = CONFIG_NABTO_DEVICE_NETWORK_THREAD_STACK_SIZE;

void nm_select_unix_run(struct nm_select_unix* ctx)
{
    esp_pthread_cfg_t pthreadConfig = esp_pthread_get_default_config();
    pthreadConfig.stack_size = NETWORK_THREAD_STACK_SIZE;
    pthreadConfig.thread_name = "Network";

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
}

void nm_select_unix_notify(struct nm_select_unix* ctx)
{
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
