#include "app/drivers/button.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/zbus/zbus.h"
#include <stdbool.h>
#include <stdint.h>

#include "lora_udp.h"

LOG_MODULE_REGISTER(relay_control, LOG_LEVEL_DBG);

ZBUS_CHAN_DEFINE(output_chan, struct chan_out, NULL, NULL, ZBUS_OBSERVERS(output_sub), {0});
ZBUS_MSG_SUBSCRIBER_DEFINE(output_sub);

ZBUS_CHAN_DECLARE(want_chan);
ZBUS_CHAN_DECLARE(want_evt_chan);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec outputs[] = {GPIO_DT_SPEC_GET(DT_ALIAS(saida0), gpios),
					      GPIO_DT_SPEC_GET(DT_ALIAS(saida1), gpios)};

static const struct gpio_dt_spec inputs[] = {GPIO_DT_SPEC_GET(DT_ALIAS(input0), gpios),
					     GPIO_DT_SPEC_GET(DT_ALIAS(input1), gpios)};

void button_handler(int id, enum button_evt evt)
{
	struct chan_out chan_msg;
	LOG_DBG("Button%d %s", id, helper_button_evt_str(evt));
	bool want_evt = true;

	gpio_pin_set_dt(&led, evt);

	if (evt != BUTTON_EVT_RELEASED) {
		return;
	}

	if (zbus_chan_read(&output_chan, &chan_msg, K_MSEC(200))) {
		LOG_ERR("Failed to read chan");
	}

	chan_msg.outputs[id] = !chan_msg.outputs[id];
	LOG_INF("Changed output%d to %s", id, chan_msg.outputs[id] ? "Enabled" : "Disabled");

	if (zbus_chan_pub(&want_chan, &chan_msg, K_MSEC(200))) {
		LOG_ERR("Failed to pub chan");
	}

	want_evt = true;
	zbus_chan_pub(&want_evt_chan, &want_evt, K_MSEC(200));
}

void output_handler(void *arg0, void *arg1, void *arg2)
{
	const struct zbus_channel *chan;
	struct chan_out state;
	struct chan_out old;

	static const struct gpio_dt_spec *out;
	bool *state_val;
	bool *old_val;

	for (int i = 0; i < NUM_OF_OUTPUTS; i++) {

		button_init(i, &inputs[i], button_handler);

		if (gpio_pin_configure_dt(&outputs[i], GPIO_OUTPUT_INACTIVE) != 0) {
			LOG_ERR("Failed to config output0");
			return;
		}
		LOG_INF("Registered button handler of ID=%d", i);
	}

	while (true) {
		if (zbus_sub_wait_msg(&output_sub, &chan, &state, K_FOREVER) != 0) {
			continue;
		}

		for (int i = 0; i < NUM_OF_OUTPUTS; i++) {
			out = &outputs[i];
			state_val = &state.outputs[i];
			old_val = &old.outputs[i];

			if (*old_val == *state_val) {
				continue;
			}

			LOG_WRN("[UPDATE] output%d %s", i, *state_val ? "enabled" : "disabled");

			gpio_pin_set_dt(out, *state_val);
			*old_val = *state_val;
		}
	}
}
K_THREAD_DEFINE(output_thread, 1024, output_handler, NULL, NULL, NULL, 5, 0, 0);

int init_relays()
{
	LOG_DBG("Initializing led");
	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) != 0) {
		LOG_ERR("Failed to config pin");
	}

	return 0;
}

SYS_INIT(init_relays, APPLICATION, 5);
