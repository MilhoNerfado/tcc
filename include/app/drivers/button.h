//
// Created by milho on 11/4/24.
//

#ifndef BUTTON_H
#define BUTTON_H

#include <zephyr/drivers/gpio.h>

enum button_evt {
	BUTTON_EVT_PRESSED,
	BUTTON_EVT_RELEASED
};

typedef void (*button_event_handler_t)(enum button_evt evt);

int button_init(int id, const struct gpio_dt_spec *button, button_event_handler_t handler);

char *helper_button_evt_str(enum button_evt evt);

#endif // BUTTON_H
