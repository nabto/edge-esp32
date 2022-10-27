#include "nabto_esp32_util.h"

#include <nn/log.h>

#include <string.h>

#include "esp_log.h"

#include "stdio.h"

void nn_log_print_function(void* userData, enum nn_log_severity severity, const char* module, const char* file, int line, const char* fmt, va_list args)
{
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    switch(severity) {
        case NN_LOG_SEVERITY_ERROR: ESP_LOGE(module, "%s", buffer); break;
        case NN_LOG_SEVERITY_WARN: ESP_LOGW(module, "%s", buffer); break;
        case NN_LOG_SEVERITY_INFO: ESP_LOGI(module, "%s", buffer); break;
        case NN_LOG_SEVERITY_TRACE: ESP_LOGV(module, "%s", buffer); break;
    }
}

void nabto_esp32_util_nn_log_init(struct nn_log* logger)
{
    nn_log_init(logger, nn_log_print_function, NULL);
}
