#ifndef _THERMOSTAT_LED_H_
#define _THERMOSTAT_LED_H_

#include <stdbool.h>
#include "thermostat_state_data.h"

// Set a led on the board to on when the power is set to on for the heatpump.
void thermostat_led_init();

void thermostat_led_update(struct thermostat_state_data* data);

#endif
