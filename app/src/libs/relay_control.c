#include "app/drivers/button.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"
#include "zephyr/zbus/zbus.h"
#include <stdbool.h>
#include <stdint.h>

#include "lora_udp.h"

LOG_MODULE_REGISTER(relay_control);

ZBUS_CHAN_DEFINE(output_chan, struct chan_out, NULL, NULL, ZBUS_OBSERVERS_EMPTY, {});

ZBUS_MSG_SUBSCRIBER_DEFINE(output_sub);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec outputs[] = {GPIO_DT_SPEC_GET(DT_ALIAS(saida0), gpios),
					      GPIO_DT_SPEC_GET(DT_ALIAS(saida1), gpios)};

static const struct gpio_dt_spec inputs[] = {GPIO_DT_SPEC_GET(DT_ALIAS(input0), gpios),
					     GPIO_DT_SPEC_GET(DT_ALIAS(input1), gpios)};

void button_handler(int id, enum button_evt evt)
{
	LOG_INF("Button0 %s", helper_button_evt_str(evt));

	if (evt != BUTTON_EVT_RELEASED) {
		return;
	}

	zbus_chan_claim(&output_chan, K_MSEC(200));

	struct chan_out *chan_msg = zbus_chan_msg(&output_chan);
	bool *state = &chan_msg->outputs[id];

	*state = !state;
	LOG_INF("Changed output0 to %s", *state ? "Disabled" : "Enabled");

	zbus_chan_finish(&output_chan);
	zbus_chan_notify(&output_chan, K_MSEC(200));
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
		LOG_WRN("[UPDATE] output%d %s", i, state.outputs[i] ? "disabled" : "enabled");

		button_init(i, &inputs[i], button_handler);

		if (gpio_pin_configure_dt(&outputs[i], GPIO_OUTPUT_INACTIVE) != 0) {
			LOG_ERR("Failed to config output0");
			return;
		}
	}

	while (true) {
		if (zbus_sub_wait_msg(&output_sub, &chan, &state, K_MSEC(100)) != 0) {
			continue;
		}

		for (int i = 0; i < NUM_OF_OUTPUTS; i++) {
			out = &outputs[i];
			state_val = &state.outputs[i];
			old_val = &old.outputs[i];

			if (*old_val == *state_val) {
				continue;
			}

			LOG_WRN("[UPDATE] output%d %s", i, state_val ? "disabled" : "enabled");

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
