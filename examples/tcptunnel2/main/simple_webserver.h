#ifndef _SIMPLE_WEBSERVER_H_
#define _SIMPLE_WEBSERVER_H_

#include <esp_http_server.h>
#include <nn/log.h>


httpd_handle_t start_webserver(struct nn_log* logger);
void stop_webserver(httpd_handle_t server);
    

#endif

