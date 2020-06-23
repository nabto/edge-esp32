
#include "simple_webserver.h"


static struct nn_log* web_logger;
static const char* LOGM = "simple_webserver";

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
 * Hello world on http://<host>/uri
 */
esp_err_t get_handler_hello(httpd_req_t *req)
{
    /* Send a simple response */
    const char* resp = "<!DOCTYPE html><html><body><h1>Hello world test!<h1></body></html>";
    httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

/* 
 * 1m stream of bytes on http://<host>/1m for stream performance testing
 */
esp_err_t get_handler_1m(httpd_req_t *req)
{

/* Send a simple response */
    
    // Create a 1k byte buffer chuck
    char resp_1k[1000];
    int len = 1000;
    for(int i=0;i<10;i++) {
        memcpy(resp_1k+(i*100), resp, 100);
    }
    resp_1k[999]=0;
    
    printf("strlen:%d 1000000/len:%d", len, 1000000/len);
    
    int64_t start_time = esp_timer_get_time();

    // Send 1m chunk
    for(int t=0; t< 1000000/len; t++) {
        int res = httpd_resp_send_chunk(req, (const char *) resp_1k, len);
        if(res != ESP_OK) {
            NN_LOG_INFO(web_logger, LOGM, "Could not send chunk %i", t);
            return ESP_FAIL;
        }
        
    }
    // send last 0 chunk to end the stream
    httpd_resp_send_chunk(req, (const char *) resp, 0);

    int64_t end_time = esp_timer_get_time();
    int64_t time = end_time - start_time;
    float kb = 1000000/1024;
    
    NN_LOG_INFO(web_logger, LOGM, "Finished in %lldms kb=%f kb/s=%f", time/1000, kb, kb/((float)time/1000000));

    return ESP_OK;
}


/* 
 * 100k stream of bytes on http://<host>/100k for stream performance testing
 */
esp_err_t get_handler_100k(httpd_req_t *req)
{

/* Send a simple response */
    
    // Create a 1k byte buffer chuck
    char resp_1k[1000];
    int len = 1000;
    for(int i=0;i<10;i++) {
        memcpy(resp_1k+(i*100), resp, 100);
    }
    resp_1k[999]=0;
    
    printf("strlen:%d 100000/len:%d", len, 100000/len);
    
    int64_t start_time = esp_timer_get_time();

    // Send 1m chunk
    for(int t=0; t< 100000/len; t++) {
        int res = httpd_resp_send_chunk(req, (const char *) resp_1k, len);
        if(res != ESP_OK) {
            NN_LOG_INFO(web_logger, LOGM, "Could not send chunk %i", t);
            return ESP_FAIL;
        }
        
    }
    // send last 0 chunk to end the stream
    httpd_resp_send_chunk(req, (const char *) resp, 0);

    int64_t end_time = esp_timer_get_time();
    int64_t time = end_time - start_time;
    float kb = 100000/1024;
    
    NN_LOG_INFO(web_logger, LOGM, "Finished in %lldms kb=%f kb/s=%f", time/1000, kb, kb/((float)time/100000));

    return ESP_OK;
}

/* 
 * 10k stream of bytes on http://<host>/10k for stream performance testing
 */
esp_err_t get_handler_10k(httpd_req_t *req)
{

/* Send a simple response */
    
    // Create a 1k byte buffer chuck
    char resp_1k[1000];
    int len = 1000;
    for(int i=0;i<10;i++) {
        memcpy(resp_1k+(i*100), resp, 100);
    }
    resp_1k[999]=0;
    
    printf("strlen:%d 10000/len:%d", len, 10000/len);
    
    int64_t start_time = esp_timer_get_time();

    // Send 1m chunk
    for(int t=0; t< 10000/len; t++) {
        int res = httpd_resp_send_chunk(req, (const char *) resp_1k, len);
        if(res != ESP_OK) {
            NN_LOG_INFO(web_logger, LOGM, "Could not send chunk %i", t);
            return ESP_FAIL;
        }
        
    }
    // send last 0 chunk to end the stream
    httpd_resp_send_chunk(req, (const char *) resp, 0);

    int64_t end_time = esp_timer_get_time();
    int64_t time = end_time - start_time;
    float kb = 10000/1024;
    
    NN_LOG_INFO(web_logger, LOGM, "Finished in %lldms kb=%f kb/s=%f", time/1000, kb, kb/((float)time/10000));

    return ESP_OK;
}


/* URI handler structure for GET /uri */
httpd_uri_t uri_get_hello = {
    .uri      = "/hello",
    .method   = HTTP_GET,
    .handler  = get_handler_hello,
    .user_ctx = NULL
};


/* URI handler structure for GET /1m */
httpd_uri_t uri_get_1m = {
    .uri      = "/1m",
    .method   = HTTP_GET,
    .handler  = get_handler_1m,
    .user_ctx = NULL
};

/* URI handler structure for GET /100k */
httpd_uri_t uri_get_100k = {
    .uri      = "/100k",
    .method   = HTTP_GET,
    .handler  = get_handler_100k,
    .user_ctx = NULL
};

/* URI handler structure for GET /10k */
httpd_uri_t uri_get_10k = {
    .uri      = "/10k",
    .method   = HTTP_GET,
    .handler  = get_handler_10k,
    .user_ctx = NULL
};

/* 
 * Function for starting the webserver
 */
httpd_handle_t start_webserver(struct nn_log* logger)
{

    web_logger = logger;
    
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    
    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &uri_get_hello);
        httpd_register_uri_handler(server, &uri_get_1m);
        httpd_register_uri_handler(server, &uri_get_100k);
        httpd_register_uri_handler(server, &uri_get_10k);
    }
    /* If server failed to start, handle will be NULL */

    if(server != NULL) {
        NN_LOG_INFO(web_logger, LOGM, "Webserver created");
    }
    
    return server;
}

/*
 * Function for stopping the webserver (not used)
 */
void stop_webserver(httpd_handle_t server)
{
    if (server) {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}
