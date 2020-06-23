#include "device_config.h"

#include "json_config.h"

#include <cjson/cJSON.h>

#include <string.h>
#include <stdlib.h>

// Simple container for important setup
// CONFIG_NABTO_* variables are setup in 'idf.py menuconfig'


bool load_device_config_esp32(struct device_config* dc, struct nn_log* logger)
{

    dc->productId = strdup(CONFIG_NABTO_PRODUCT);
    dc->deviceId = strdup(CONFIG_NABTO_DEVICE);
    dc->server = strdup(CONFIG_NABTO_SERVER);
    dc->clientServerKey = strdup(CONFIG_NABTO_CLIENT_SERVERKEY);
    dc->clientServerUrl = strdup(CONFIG_NABTO_CLIENT_SERVERURL);

    printf("Nabto product id : %s\n",  dc->productId);
    printf("Nabto device id : %s\n", dc->deviceId);
    printf("Nabto server : %s\n", dc->server);
    printf("Nabto clientServerKey : %s\n", dc->clientServerKey);
    printf("Nabto clientServerURL : %s\n", dc->clientServerUrl);


    return true;
}


void device_config_init(struct device_config* config)
{
    memset(config, 0, sizeof(struct device_config));
}

void device_config_deinit(struct device_config* config)
{
    free(config->productId);
    free(config->deviceId);
    free(config->server);
    free(config->clientServerKey);
    free(config->clientServerUrl);
}
