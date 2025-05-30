#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"
#include "app/drivers/button.h"

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

LOG_MODULE_REGISTER(main);

#define SELF_ID 3

#define SLEEP_TIME_MS 1000

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec relay = GPIO_DT_SPEC_GET(DT_ALIAS(saida1), gpios);

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(input0), gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(DT_ALIAS(input1), gpios);

volatile static long int counter = 0;

static void button0_evt_handler(enum button_evt evt)
{
	gpio_pin_set_dt(&relay, 0);
	gpio_pin_set_dt(&led, 0);
}

static void button1_evt_handler(enum button_evt evt)
{
	// gpio_pin_toggle_dt(&led);
	gpio_pin_set_dt(&relay, 1);
	gpio_pin_set_dt(&led, 1);
	counter++;
}

void relay_cb(uint8_t sender_id, uint8_t *data, size_t data_len, uint8_t *response,
	      size_t *response_size)
{
	LOG_INF("sender id: %d | data_len: %d\n", sender_id, data_len);

	LOG_INF("data: %p | response: %p | response_size: %p\n", data, response, response_size);

	LOG_WRN("Response: %.*s", data_len, (char *)data);

	if (data_len == 0) {
		return;
	}

	if (data[0] == '0') {
		printf("Turning Relay OFF\n");
		gpio_pin_set_dt(&relay, 0);
		gpio_pin_set_dt(&led, 0);

		memset(response, 0, *response_size);
		snprintf((char *)response, *response_size, "OFF");
		*response_size = 3;
	}

	if (data[0] == '1') {
		printf("Turning Relay ON\n");
		gpio_pin_set_dt(&relay, 1);
		gpio_pin_set_dt(&led, 1);

		memset(response, 0, *response_size);
		snprintf((char *)response, *response_size, "ON");
		*response_size = 2;
	}
}

int main(void)
{
	lora_tcp_init(DEVICE_DT_GET(DEFAULT_RADIO_NODE), SELF_ID, relay_cb);

	lora_tcp_register(1);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- ACTUATOR --- | ID: %d --- ", SELF_ID);

	int ret;

	if (!gpio_is_ready_dt(&led)) {
		printk("Error: led device %s is not ready\n", led.port->name);
		return 0;
	}
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		printk("Error: led config device %s is not ready\n", led.port->name);

		return 0;
	}

	if (!gpio_is_ready_dt(&relay)) {
		printk("Error: relay device %s is not ready\n", relay.port->name);
		return 0;
	}
	ret = gpio_pin_configure_dt(&relay, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		printk("Error %d: relay config device %s\n", ret, relay.port->name);
		return 0;
	}

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

	k_msleep(20000);

	while (1) {
		LOG_INF("[%d]", counter);
		k_msleep(20);
	}
}
