#include <api/nabto_platform.h>

#include <modules/dtls/nm_dtls_cli.h>
#include <modules/dtls/nm_dtls_srv.h>
#include <modules/dns/unix/nm_unix_dns.h>
#include <modules/timestamp/unix/nm_unix_timestamp.h>
#include <modules/select_unix/nm_select_unix.h>
#include <modules/dtls/nm_random.h>

#include "esp32_dns.h"
#include "esp32_logging.h"

struct nm_select_unix selectCtx;

np_error_code nabto_device_init_platform_modules(struct np_platform* pl)
{
    np_error_code ec;
    esp32_log_init();
    np_communication_buffer_init(pl);
    ec = nm_select_unix_init(&selectCtx, pl);
    if (ec != NABTO_EC_OK) {
        return ec;
    }
    nm_unix_ts_init(pl);
    esp32_dns_init(pl);
    nm_dtls_cli_init(pl);
    nm_dtls_srv_init(pl);
    return NABTO_EC_OK;
}

void nabto_device_deinit_platform_modules(struct np_platform* pl)
{
    nm_random_deinit(pl);
}


int nabto_device_platform_inf_wait()
{
    return nm_select_unix_inf_wait(&selectCtx);
}

void nabto_device_platform_read(int nfds)
{
    nm_select_unix_read(&selectCtx, nfds);
}

void nabto_device_platform_close(struct np_platform* pl)
{
    nm_select_unix_close(&selectCtx);
}

void nabto_device_platform_signal(struct np_platform* pl)
{
    nm_select_unix_break_wait(&selectCtx);
}

bool nabto_device_platform_finished() {
    return nm_select_unix_finished(&selectCtx);
}
