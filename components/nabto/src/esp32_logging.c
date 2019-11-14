
#include "esp32_logging.h"
#include <nabto_types.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define NM_UNIX_LOGGING_FILE_LENGTH 24

static void esp32_log (uint32_t severity, uint32_t module, uint32_t line, const char* file, const char* fmt, va_list args);
static void esp32_log_buf(uint32_t severity, uint32_t module, uint32_t line, const char* file, const uint8_t* buf, size_t len);


static uint32_t logSeverityFilter = NABTO_LOG_SEVERITY_LEVEL_INFO;

void esp32_log_init()
{
    np_log.log = &esp32_log;
    np_log.log_buf = &esp32_log_buf;

    logSeverityFilter = NABTO_LOG_SEVERITY_LEVEL_TRACE;


}

void esp32_log_buf(uint32_t severity, uint32_t module, uint32_t line, const char* file, const uint8_t* buf, size_t len){
    // TODO
    return;
}

void esp32_log (uint32_t severity, uint32_t module, uint32_t line, const char* file, const char* fmt, va_list args)
{
    if(((logSeverityFilter & severity) && ((NABTO_LOG_MODULE_FILTER & module) || module == 0))) {
        time_t sec;
        unsigned int ms;
        struct timeval tv;
        struct tm tm;
        gettimeofday(&tv, NULL);
        sec = tv.tv_sec;
        ms = tv.tv_usec/1000;

        localtime_r(&sec, &tm);

        size_t fileLen = strlen(file);
        char fileTmp[NM_UNIX_LOGGING_FILE_LENGTH+4];
        if(fileLen > NM_UNIX_LOGGING_FILE_LENGTH) {
            strcpy(fileTmp, "...");
            strcpy(fileTmp + 3, file + fileLen - NM_UNIX_LOGGING_FILE_LENGTH);
        } else {
            strcpy(fileTmp, file);
        }
        char level[6];
        switch(severity) {
            case NABTO_LOG_SEVERITY_ERROR:
                strcpy(level, "ERROR");
                break;
            case NABTO_LOG_SEVERITY_WARN:
                strcpy(level, "_WARN");
                break;
            case NABTO_LOG_SEVERITY_INFO:
                strcpy(level, "_INFO");
                break;
            case NABTO_LOG_SEVERITY_TRACE:
                strcpy(level, "TRACE");
                break;
        }

        printf("%02u:%02u:%02u:%03u %s(%03u)[%s] ",
               tm.tm_hour, tm.tm_min, tm.tm_sec, ms,
               fileTmp, line, level);
        vprintf(fmt, args);
        printf("\n");
    }
}
