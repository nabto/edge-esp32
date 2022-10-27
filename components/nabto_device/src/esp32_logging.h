#ifndef _NM_UNIX_LOGGING_H_
#define _NM_UNIX_LOGGING_H_

#include <nabto/nabto_device.h>
#include <nn/log.h>

void esp32_logging_init(NabtoDevice* device, struct nn_log* logger);

/*
void nm_unix_log (uint32_t severity, uint32_t module, uint32_t line, const char* file, const char* fmt, va_list args);
void nm_unix_log_buf(uint32_t severity, uint32_t module, uint32_t line, const char* file, const uint8_t* buf, size_t len);
*/

#endif //NM_UNIX_LOGGING_H_
