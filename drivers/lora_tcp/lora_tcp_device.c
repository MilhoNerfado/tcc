//
// Created by milho on 8/12/24.
//

#include "lora_tcp_device.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

LOG_MODULE_REGISTER(lora_tcp_device);

static struct {
	struct lora_tcp_device device;
	struct lora_tcp_device device_list[CONFIG_LORA_TCP_DEVICE_MAX];
	bool is_init;
} self = {
	.device_list = {0},
	.is_init = false,
};

void lora_tcp_device_self_set(uint8_t id, uint8_t key_id)
{
	self.device.id = id;
	self.device.key_id = key_id;

	self.is_init = true;
}

struct lora_tcp_device *lora_tcp_device_self_get(void)
{
	if (!self.is_init) {
		return NULL;
	}

	return &self.device;
}

int lora_tcp_device_register(uint8_t id, uint8_t key_id)
{

	for (size_t i = 0; i < CONFIG_LORA_TCP_DEVICE_MAX; i++) {
		if (self.device_list[i].id == id && self.device_list[i].is_registered == true) {
			LOG_WRN("Device ID already registered");
			return -EEXIST;
		}
		if (self.device_list[i].is_registered == true) {
			continue;
		}

		self.device_list[i].id = id;
		self.device_list[i].key_id = key_id;
		self.device_list[i].is_registered = true;
	}

	return 0;
}

int lora_tcp_device_unregister(uint8_t id)
{
	struct lora_tcp_device *dev = lora_tcp_device_get_by_id(id);

	if (dev == NULL) {
		return -ENODEV;
	}

	dev->id = 0;
	dev->key_id = 0;
	dev->is_registered = false;

	return 0;
}

struct lora_tcp_device *lora_tcp_device_get_by_id(uint8_t id)
{
	for (size_t i = 0; i < CONFIG_LORA_TCP_DEVICE_MAX; i++) {
		if (self.device_list[i].id == id && self.device_list[i].is_registered) {
			return &self.device_list[i];
		}
	}

	return NULL;
}
