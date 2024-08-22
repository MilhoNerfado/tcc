//
// Created by milho on 8/1/24.
//

#ifndef RELAY_H
#define RELAY_H

#include <zephyr/drivers/gpio.h>

int relay_init(struct gpio_dt_spec *relay);
int relay_get(bool *state);
int relay_set(bool state);
int relay_toggle(void);

#endif // RELAY_H
