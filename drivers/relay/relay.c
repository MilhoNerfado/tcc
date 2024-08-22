//
// Created by milho on 8/1/24.
//

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(relay_driver);

static struct {
	bool is_init;
	struct gpio_dt_spec *relay;
	bool relay_state;
} self = {
	.is_init = false,
	.relay_state = false,
};

int relay_init(struct gpio_dt_spec *relay)
{
	int err;

	if (self.is_init) {
		return 0;
	}

	if (relay == NULL) {
		LOG_ERR("Relay pointer NULL");
		return -EINVAL;
	}

	if (!gpio_is_ready_dt(relay)) {
		LOG_ERR("GPIO is not ready");
		return -EIO;
	}

	err = gpio_pin_configure_dt(relay, GPIO_OUTPUT_ACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure GPIO | err: %d", err);
		return err;
	}

	gpio_pin_set_dt(relay, false);

	self.relay = relay;
	self.relay_state = false;

	self.is_init = true;

	return 0;
}

int relay_get(bool *state)
{
	*state = self.relay_state;
	return 0;
}

int relay_set(bool state)
{
	int err;

	if (!self.is_init) {
		LOG_ERR("Driver not initialized");
		return -EACCES;
	}

	err = gpio_pin_set_dt(self.relay, state);
	if (err != 0) {
		LOG_WRN("Failed to set relay state");
		return -EIO;
	}

	return 0;
}

int relay_toggle(void)
{
	int err;

	if (!self.is_init) {
		LOG_ERR("Driver not initialized");
		return -EACCES;
	}

	err = gpio_pin_set_dt(self.relay, !self.relay_state);
	if (err != 0) {
		LOG_WRN("Failed to toggle relay state");
		return -EIO;
	}

	self.relay_state = !self.relay_state;

	return 0;
}
