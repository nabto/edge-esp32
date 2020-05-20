#include "device_config.h"

#include "json_config.h"

#include <cjson/cJSON.h>

#include <string.h>
#include <stdlib.h>

static const char* LOGM = "device_config";

bool load_device_config(nvs_handle_thandle, const char* fileName, struct device_config* dc, struct nn_log* logger)
{
    cJSON* config;
    if (!json_config_load(nvs_handle_thandle, fileName, &config, logger)) {
        return false;
    }

    cJSON* productId = cJSON_GetObjectItem(config, "ProductId");
    cJSON* deviceId = cJSON_GetObjectItem(config, "DeviceId");
    cJSON* server = cJSON_GetObjectItem(config, "Server");
    cJSON* client = cJSON_GetObjectItem(config, "Client");

    if (!cJSON_IsString(productId) ||
        !cJSON_IsString(deviceId) ||
        !cJSON_IsString(server))
    {
        NN_LOG_ERROR(logger, LOGM, "Missing required device config options");
        return false;
    }

    dc->productId = strdup(productId->valuestring);
    dc->deviceId = strdup(deviceId->valuestring);
    dc->server = strdup(server->valuestring);

    if (cJSON_IsObject(client)) {
        cJSON* clientServerKey = cJSON_GetObjectItem(client, "ServerKey");
        cJSON* clientServerUrl = cJSON_GetObjectItem(client, "ServerUrl");
        if (!cJSON_IsString(clientServerKey) ||
            !cJSON_IsString(clientServerUrl))
        {
            NN_LOG_ERROR(logger, LOGM, "Invalid client settings in device config file");
            return false;
        }

        dc->clientServerKey = strdup(clientServerKey->valuestring);
        dc->clientServerUrl = strdup(clientServerUrl->valuestring);
    }

    cJSON_Delete(config);

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
