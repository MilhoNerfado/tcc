#include "app/drivers/button.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/zbus/zbus.h"
#include <stdbool.h>
#include <stdint.h>

#include "lora_udp.h"

LOG_MODULE_REGISTER(relay_control);

ZBUS_CHAN_DEFINE(output_chan, struct chan_out, NULL, NULL, ZBUS_OBSERVERS_EMPTY, {0});

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec outputs[] = {GPIO_DT_SPEC_GET(DT_ALIAS(saida0), gpios),
					      GPIO_DT_SPEC_GET(DT_ALIAS(saida1), gpios)};

static const struct gpio_dt_spec inputs[] = {GPIO_DT_SPEC_GET(DT_ALIAS(input0), gpios),
					     GPIO_DT_SPEC_GET(DT_ALIAS(input1), gpios)};

void button_handler(int id, enum button_evt evt)
{
	struct chan_out chan_msg;
	LOG_DBG("Button%d %s", id, helper_button_evt_str(evt));

	gpio_pin_set_dt(&led, evt);

	if (evt != BUTTON_EVT_RELEASED) {
		return;
	}

	if (zbus_chan_read(&output_chan, &chan_msg, K_MSEC(200))) {
		LOG_ERR("Failed to read chan");
	}

	chan_msg.outputs[id] = !chan_msg.outputs[id];
	LOG_INF("Changed output%d to %s", id, chan_msg.outputs[id] ? "Disabled" : "Enabled");

	if (zbus_chan_pub(&output_chan, &chan_msg, K_MSEC(200))) {
		LOG_ERR("Failed to pub chan");
	}
}

int init_relays()
{
	LOG_DBG("Initializing led");
	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) != 0) {
		LOG_ERR("Failed to config pin");
	}

	button_init(0, &inputs[0], button_handler);
	button_init(1, &inputs[1], button_handler);

	return 0;
}

SYS_INIT(init_relays, POST_KERNEL, 4);
