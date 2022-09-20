#ifndef _JSON_CONFIG_H_
#define _JSON_CONFIG_H_

#include <stdbool.h>
#include <cjson/cJSON.h>

#include "nvs_flash.h"

#ifdef __cplusplus
extern "C" {
#endif
    
    struct nn_log;
    
    bool json_config_exists_handle(nvs_handle_t handle, const char* nvsName);
    bool json_config_exists(const char* nvsName);
    bool json_config_load_handle(nvs_handle_t handle, const char* nvsName, cJSON** config, struct nn_log* logger);
    bool json_config_save_handle(nvs_handle_t handle, const char* nvsName, cJSON* config);
    bool json_config_load(const char* nvsName, cJSON** config, struct nn_log* logger);
    bool json_config_save(const char* nvsName, cJSON* config);
    
#ifdef __cplusplus
} //extern "C"
#endif

#endif
