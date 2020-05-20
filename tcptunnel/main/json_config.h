#ifndef _JSON_CONFIG_H_
#define _JSON_CONFIG_H_

#include <stdbool.h>
#include <cjson/cJSON.h>

#ifdef __cplusplus
extern "C" {
#endif
    
    struct nn_log;
    
    bool json_config_exists(nvs_handle_t handle, const char* nvsName);
    bool json_config_load(nvs_handle_t handle, const char* nvsName, cJSON** config, struct nn_log* logger);
    bool json_config_save(nvs_handle_t handle, const char* nvsName, cJSON* config);
    
#ifdef __cplusplus
} //extern "C"
#endif

#endif
