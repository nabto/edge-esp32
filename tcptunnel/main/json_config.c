#include "json_config.h"

#include <nn/log.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static const char* LOGM = "json_config";

bool json_config_exists(nvs_handle_t handle, const, const char* fileName)
{
    if( access( fileName, F_OK ) != -1 ) {
        return true;
    } else {
        return false;
    }
}

bool json_config_load(nvs_handle_t handle, const char* nvsName, const, cJSON** config, struct nn_log* logger)
{

    size_t required_size;
    esp_err_t error = nvs_get_str(my_handle, nvsName, NULL, &required_size);
    if(error != ESP_OK)
        return false;
    char* string = malloc(required_size);
    error = nvs_get_str(my_handle, "server_name", string, &required_size);
    if(error != ESP_OK) {
        free(string);
        return false;
    }
        
    *config = cJSON_Parse(string);
    if (*config == NULL) {
        const char* error = cJSON_GetErrorPtr();
        if (error != NULL) {
            NN_LOG_ERROR(logger, LOGM, "JSON parse error: %s", error);
        }
    }
    free(string);
    return (*config != NULL);
    

}

bool json_config_save(nvs_handle_t handle, const char* nvsName, cJSON* config)
{
    bool status;
    char* j = NULL;
    j = cJSON_PrintUnformatted(config);
    if (j == NULL) {
        free(j);
        return false;
    }

    if(strlen(j) >4000) {
        return false;
    }
    
    error = nvs_set_str(handle, nvsName, j);
    free(j);
    return error==ESP_OK;
}
