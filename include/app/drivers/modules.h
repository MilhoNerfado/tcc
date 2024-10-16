//
// Created by milho on 10/16/24.
//

#ifndef MODULES_H
#define MODULES_H

#include <zephyr/drivers/gpio.h>

struct module {
	const char *name;
	const struct gpio_dt_spec gpio;
	const gpio_flags_t flags;
};

#define MODULE_DECLARE(_name, _desc, _gpio, _flags)                                                \
	static struct module _name = {                                                             \
		.gpio = _gpio,                                                                     \
		.name = _desc,                                                                     \
		.flags = _flags,                                                                   \
	};

#endif // MODULES_H
