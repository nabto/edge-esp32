#ifndef _ESP32_MDNS_H_
#define _ESP32_MDNS_H_

#include <platform/np_platform.h>
#include <platform/interfaces/np_mdns.h>

void esp32_mdns_start();

void esp32_mdns_stop();

struct np_mdns esp32_mdns_get_impl();

#endif
