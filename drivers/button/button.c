//
// Created by milho on 11/4/24.
//

#include <zephyr/kernel.h>

#include "app/drivers/button.h"

#define SW0_NODE DT_ALIAS(input0)

struct button_data {
	struct gpio_callback button_cb_data;
	const struct gpio_dt_spec *button;
	button_event_handler_t user_cb;
};

static struct button_data button_list[2];

static void cooldown_expired0(struct k_work *work)
{
	ARG_UNUSED(work);

	int val = gpio_pin_get_dt(button_list[0].button);
	enum button_evt evt = val ? BUTTON_EVT_PRESSED : BUTTON_EVT_RELEASED;
	if (button_list[0].user_cb) {
		button_list[0].user_cb(evt);
	}
}

static void cooldown_expired1(struct k_work *work)
{
	ARG_UNUSED(work);

	int val = gpio_pin_get_dt(button_list[1].button);
	enum button_evt evt = val ? BUTTON_EVT_PRESSED : BUTTON_EVT_RELEASED;
	if (button_list[1].user_cb) {
		button_list[1].user_cb(evt);
	}
}

static K_WORK_DELAYABLE_DEFINE(cooldown_work0, cooldown_expired0);
static K_WORK_DELAYABLE_DEFINE(cooldown_work1, cooldown_expired1);

void button_pressed0(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_reschedule(&cooldown_work0, K_MSEC(15));
}

void button_pressed1(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_reschedule(&cooldown_work1, K_MSEC(15));
}

void *cb_list[] = {
	button_pressed0,
	button_pressed1,
};

int button_init(int id, const struct gpio_dt_spec *button, button_event_handler_t handler)
{
	int err = -1;

	if (!handler) {
		return -EINVAL;
	}

	button_list[id].user_cb = handler;
	button_list[id].button = button;

	if (!device_is_ready(button->port)) {
		return -EIO;
	}

	err = gpio_pin_configure_dt(button, GPIO_INPUT);
	if (err) {
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(button, GPIO_INT_EDGE_BOTH);
	if (err) {
		return err;
	}

	gpio_init_callback(&button_list[id].button_cb_data, cb_list[id], BIT(button->pin));
	err = gpio_add_callback(button->port, &button_list[id].button_cb_data);
	if (err) {
		return err;
	}

	return 0;
}

char *helper_button_evt_str(enum button_evt evt)
{
	switch (evt) {
	case BUTTON_EVT_PRESSED:
		return "Pressed";
	case BUTTON_EVT_RELEASED:
		return "Released";
	default:
		return "Unknown";
	}
}