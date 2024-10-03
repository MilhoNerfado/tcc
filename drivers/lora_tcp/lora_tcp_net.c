//
// Created by milho on 10/2/24.
//

#include "lora_tcp_net.h"

#include "lora_tcp_device.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>


LOG_MODULE_REGISTER(lora_tcp_net);

#define CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE (4 * 1024)

static struct lora_modem_config config;
static const struct device *lora_dev;

int lora_tcp_net_init(const struct device * dev)
{
	if (dev == NULL) {
		LOG_ERR("Device is NULL");
		return -ENODEV;
	}

	lora_dev = dev;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return -ENODEV;
	}

	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_10;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 14;
	config.tx = false;

	return 0;
}

void inner_thread(void *, void *, void *)
{
	k_sleep(K_MSEC(5000));
	printf("lora_tcp_net_init\n");
	printf("lora_tcp_net_init\n");
	k_sleep(K_FOREVER);
}

K_THREAD_DEFINE(inner_tid, CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE, inner_thread, NULL, NULL, NULL, 5,
		0, 0);