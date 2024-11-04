//
// Created by milho on 10/16/24.
//

#ifndef MODULES_H
#define MODULES_H

#include <zephyr/drivers/gpio.h>

struct module {
	const struct gpio_dt_spec gpio;
};

#define MODULE_DECLARE(_device_id, _device)                                                        \
	STRUCT_SECTION_ITERABLE(modules_iter_devices, _device_id) = {                              \
		.gpio = _device,                                                                   \
	}

void modules_init(void)
{
//	STRUCT_SECTION_FOREACH(modules_iter_devices, _device)
//	{
//		gpio_pin_configure(_device.port, _device.pin, _device.flags);
//	}
}


#endif // MODULES_H
