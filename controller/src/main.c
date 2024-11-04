#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"

#include "app/drivers/button.h"

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

LOG_MODULE_REGISTER(main);

#define SELF_ID 1

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(input0), gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(input1), gpios);

void relay(uint8_t id, uint8_t *data, size_t data_len, uint8_t *response, size_t *response_size)
{
	LOG_WRN("Relay ran | data lenght: %lu", data_len);
}

static void button0_evt_handler(enum button_evt evt)
{
	static const char msg[] = "0";
	lora_tcp_dumb_send(3, msg, sizeof(msg));
	gpio_pin_set_dt(&led, 0);
	printf("OFF");
}

static void button1_evt_handler(enum button_evt evt)
{
	static const char msg[] = "1";
	lora_tcp_dumb_send(3, msg, sizeof(msg));
	gpio_pin_set_dt(&led, 1);
	printf("ON");
}

int main(void)
{
	lora_tcp_init(DEVICE_DT_GET(DEFAULT_RADIO_NODE), SELF_ID, relay);

	lora_tcp_register(3);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- CONTROLLER | ID: %d --- ", SELF_ID);

	int ret;

	ret = button_init(0, &button0, button0_evt_handler);
	if (ret) {
		printk("Button0 Init failed: %d\n", ret);
		return 0;
	}
	ret = button_init(1, &button1, button1_evt_handler);
	if (ret) {
		printk("Button1 Init failed: %d\n", ret);
		return 0;
	}

	k_sleep(K_FOREVER);

	return 0;
}
