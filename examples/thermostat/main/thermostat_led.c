#include "thermostat_led.h"

#include "driver/gpio.h"

#define EXAMPLE_POWER_PIN 22

void thermostat_led_init()
{
    gpio_set_direction(EXAMPLE_POWER_PIN, GPIO_MODE_OUTPUT);
}

void thermostat_led_update(struct thermostat_state_data* data)
{
    gpio_set_level(EXAMPLE_POWER_PIN, data->power?1:0);
}
