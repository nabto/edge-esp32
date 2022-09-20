#include "json_config.h"

#include <nn/log.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static const char* LOGM = "json_config";

bool json_config_exists_handle(nvs_handle_t handle, const char* nvsName)
{

    size_t required_size;
    esp_err_t error = nvs_get_str(handle, nvsName, NULL, &required_size);
    
    if(error != ESP_OK) {
        return false;
    }
    return true;
    
}
bool json_config_exists(const char* nvsName)
{

    nvs_handle_t handle;
    nvs_open("nabto", NVS_READWRITE, &handle);
    bool status = json_config_exists_handle(handle, nvsName);
    nvs_close(handle);

    return status;
    
}


bool json_config_load(const char* nvsName, cJSON** config, struct nn_log* logger) {
    nvs_handle_t handle;
    nvs_open("nabto", NVS_READWRITE, &handle);
    json_config_load_handle(handle, nvsName, config, logger);
    nvs_close(handle);
    return true;
}

bool json_config_load_handle(nvs_handle_t handle, const char* nvsName, cJSON** config, struct nn_log* logger)
{

    size_t required_size;
    esp_err_t error = nvs_get_str(handle, nvsName, NULL, &required_size);
    if(error != ESP_OK)
        return false;
    char* string = malloc(required_size);

    error = nvs_get_str(handle, nvsName, string, &required_size);
    if(error != ESP_OK) {
        free(string);
        return false;
    }
    NN_LOG_TRACE(logger, LOGM, "Loaded json from nvs key:%s with content: %s", nvsName, string);
   
    
    *config = cJSON_Parse(string);
    if (*config == NULL) {
        NN_LOG_TRACE(logger, LOGM, "Got NULL config");
        const char* error = cJSON_GetErrorPtr();        
        if (error != NULL) {
            NN_LOG_ERROR(logger, LOGM, "JSON parse error: %s", error);
        }
    }
    free(string);
    return (*config != NULL);
    

}


bool json_config_save(const char* nvsName, cJSON* config) {
    nvs_handle_t handle;
    bool status;
    esp_err_t error = nvs_open("nabto", NVS_READWRITE, &handle);
    ESP_ERROR_CHECK(error);
    status = json_config_save_handle(handle, nvsName, config);
    nvs_close(handle);
    return status;
}

bool json_config_save_handle(nvs_handle_t handle, const char* nvsName, cJSON* config)
{

    char* j = NULL;
    j = cJSON_PrintUnformatted(config);
    if (j == NULL) {
        free(j);
        return false;
    }

    if(strlen(j) >4000) {
        printf("Could not save nvs key:%s .. resulting string too big: %s\n", nvsName, j);
        free(j);
        return false;
    }
    printf("Create json and will store it in nvs key:%s with content: %s\n", nvsName, j);
    
    esp_err_t error = nvs_set_str(handle, nvsName, j);
    if(error != ESP_OK) {

        switch(error) {
        case ESP_ERR_NVS_INVALID_HANDLE:
            printf("ESP_ERR_NVS_INVALID_HANDLE\n");
            break;
        case ESP_ERR_NVS_READ_ONLY:
            printf("ESP_ERR_NVS_READ_ONLY\n");
            break;
        case ESP_ERR_NVS_INVALID_NAME:
            printf("ESP_ERR_NVS_INVALID_NAME\n");
            break;
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            printf("ESP_ERR_NVS_NOT_ENOUGH_SPACE\n");
            break;
        case ESP_ERR_NVS_REMOVE_FAILED:
            printf("ESP_ERR_NVS_REMOVE_FAILED\n");
            break;
        case ESP_ERR_NVS_VALUE_TOO_LONG:
            printf("ESP_ERR_NVS_VALUE_TOO_LONG\n");
            break;
        default:
            printf("Error in nvs_set_str .. could not determine error type: %d\n", error);
        }
        
        printf("Could not save nvs key:%s\n", nvsName);
    }
    free(j);
    return error==ESP_OK;
}
