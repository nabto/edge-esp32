
#include "esp32_logging.h"

#include <nabto/nabto_device.h>
#include <nn/log.h>

#include <nabto_types.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define NM_UNIX_LOGGING_FILE_LENGTH 24


static void esp32_log(NabtoDeviceLogMessage* msg, void* data);
static void nn_log_function(void* userData, enum nn_log_severity severity, const char* module, const char* file, int line, const char* fmt, va_list args);


static uint32_t logMask;



void esp32_logging_init(NabtoDevice* device, struct nn_log* logger)
{
    nabto_device_set_log_callback(device, esp32_log, NULL);
    nabto_device_set_log_level(device, "trace");

    logMask = NN_LOG_SEVERITY_ERROR | NN_LOG_SEVERITY_WARN | NN_LOG_SEVERITY_INFO | NN_LOG_SEVERITY_TRACE;
    
    nn_log_init(logger, nn_log_function, NULL);
}


const char* truncated_file_name(const char* filename)
{
    size_t len = strlen(filename);
    if (len > 24) {
        return filename + (len-24);
    } else {
        return filename;
    }
}

const char* line_as_str(int line)
{
    static char buffer[32];
    if (line < 10) {
        sprintf(buffer, "%d   ", line);
    } else if (line < 100) {
        sprintf(buffer, "%d  ", line);
    } else if (line < 1000) {
        sprintf(buffer, "%d ", line);
    } else {
        sprintf(buffer, "%d", line);
    }
    return buffer;
}

const char* device_severity_as_string(NabtoDeviceLogLevel severity)
{
    switch (severity) {
        case NABTO_DEVICE_LOG_FATAL: return "FATAL";
        case NABTO_DEVICE_LOG_ERROR: return "ERROR";
        case NABTO_DEVICE_LOG_WARN:  return "WARN ";
        case NABTO_DEVICE_LOG_INFO:  return "INFO ";
        case NABTO_DEVICE_LOG_TRACE: return "TRACE";
    }
    return "NONE ";
}



void esp32_log(NabtoDeviceLogMessage* msg, void* data)
{
    printf("%s:%s %s - ", truncated_file_name(msg->file), line_as_str(msg->line), device_severity_as_string(msg->severity));
    printf("%s\n", msg->message);
}


const char* nn_log_severity_as_str(enum nn_log_severity severity)
{
    switch(severity) {
        case NN_LOG_SEVERITY_ERROR: return "ERROR";
        case NN_LOG_SEVERITY_WARN:  return "WARN ";
        case NN_LOG_SEVERITY_INFO:  return "INFO ";
        case NN_LOG_SEVERITY_TRACE: return "TRACE";
    }
    return "NONE ";
}


void nn_log_function(void* userData, enum nn_log_severity severity, const char* module, const char* file, int line, const char* fmt, va_list args)
{
    printf("LOGSTART\n");
    if ((severity & logMask) != 0) {
        printf("%s:%s %s - ", truncated_file_name(file), line_as_str(line), nn_log_severity_as_str(severity));
        vprintf(fmt, args);
        printf("\n");
    }
    printf("LOGEND\n");
}

