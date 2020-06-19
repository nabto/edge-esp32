#include "tcp_tunnel_services.h"

#include "json_config.h"

#include <cjson/cJSON.h>

#include <stdlib.h>
#include <string.h>



struct tcp_tunnel_service* tcp_tunnel_service_new()
{
    struct tcp_tunnel_service* service = calloc(1, sizeof(struct tcp_tunnel_service));
    return service;
}

void tcp_tunnel_service_free(struct tcp_tunnel_service* service)
{
    free(service->id);
    free(service->type);
    free(service->host);
    free(service);
}

bool load_tcp_tunnel_services(struct nn_vector* services, struct nn_log* logger)
{
    
    struct tcp_tunnel_service* service = tcp_tunnel_service_new();
    service->id = strdup("http");
    service->type = strdup("http");
    service->host = strdup("127.0.0.1");
    service->port = 80;
    nn_vector_push_back(services, service);
    return true;
}

