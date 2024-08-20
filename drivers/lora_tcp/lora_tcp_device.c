//
// Created by milho on 8/12/24.
//

#include "lora_tcp_device.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lora_tcp_device);

static struct lora_tcp_device device_list[CONFIG_LORA_TCP_DEVICE_MAX] = {0};

static uint64_t device_list_metadata = 0;

int lora_tcp_device_register_new(uint8_t id, uint8_t pub_id, uint8_t priv_id)
{
	if (device_list_metadata == -1) {
		LOG_WRN("Not enought space for more devices");
		return -ENOSPC;
	}

	for (int i = 0; i < MIN(CONFIG_LORA_TCP_DEVICE_MAX, sizeof(device_list_metadata)); ++i) {
		if (device_list_metadata & (1 << i)) {
			continue;
		}

		// TODO Add device to the list
		device_list[i].ids.dev_id = id;
		device_list[i].ids.priv_key_id = priv_id;
		device_list[i].ids.pub_key_id = pub_id;

		device_list_metadata += (1 << i);
	}

	return 0;
}

int lora_tcp_device_get_by_id(uint8_t id, struct lora_tcp_device **device_p)
{
	if (device_list_metadata == 0) {
		LOG_DBG("device list is empty");
		return -ENODEV;
	}

	for (int i = 0; i < MIN(CONFIG_LORA_TCP_DEVICE_MAX, sizeof(device_list_metadata)); i++) {
		if (device_list[i].ids.dev_id == id) {
			if (device_p != NULL) {
				*device_p = &device_list[i];
			}

			return 0;
		}
	}

	LOG_DBG("device not found");
	return -ENODEV;
}

void lora_tcp_device_clear_list(void)
{
	memset(device_list, 0, sizeof(struct lora_tcp_device) * CONFIG_LORA_TCP_DEVICE_MAX);
	device_list_metadata = 0;
}
