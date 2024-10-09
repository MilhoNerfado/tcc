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

void lora_tcp_device_self_set(const uint8_t id)
{
	self.device.id = id;
	self.device.is_registered = true;
}

struct lora_tcp_device *lora_tcp_device_self_get(void)
{
	if (!self.device.is_registered) {
		return NULL;
	}

	return &self.device;
}

int lora_tcp_device_register(const uint8_t id)
{
	CHECKIF(!self.device.is_registered)
	{
		LOG_ERR("Self is not registered");
		return -ENODEV;
	}

	CHECKIF(id == self.device.id) {
		LOG_ERR("Trying to register own ID as another");
		return -EINVAL;
	}

	for (size_t i = 0; i < CONFIG_LORA_TCP_DEVICE_MAX; i++) {
		if (self.device_list[i].id == id && self.device_list[i].is_registered == true) {
			LOG_WRN("Device ID already registered");
			return -EEXIST;
		}
		if (self.device_list[i].is_registered == true) {
			continue;
		}

		self.device_list[i].id = id;
		self.device_list[i].is_registered = true;
		self.device_list[i].packet.header.destination_id = id;
		self.device_list[i].packet.header.sender_id = self.device.id;
	}

	return 0;
}

int lora_tcp_device_unregister(const uint8_t id)
{
	CHECKIF(id == self.device.id) {
		LOG_ERR("Shouldn't unregister itself");
		return -EINVAL;
	}

	struct lora_tcp_device *dev = lora_tcp_device_get_by_id(id);

	if (dev == NULL) {
		return -ENODEV;
	}

	dev->id = 0;
	dev->is_registered = false;

	return 0;
}

struct lora_tcp_device *lora_tcp_device_get_by_id(const uint8_t id)
{
	for (size_t i = 0; i < CONFIG_LORA_TCP_DEVICE_MAX; i++) {
		if (self.device_list[i].id == id && self.device_list[i].is_registered) {
			return &self.device_list[i];
		}
	}

	return NULL;
}

int lora_tcp_device_get_pkt_id(const struct lora_tcp_device *device)
{
	CHECKIF(device == NULL) {
		LOG_ERR("Arg invall");
		return -EINVAL;
	}

	return device->snd_pkt_id;
}

int lora_tcp_device_update_pkt_id(struct lora_tcp_device *device)
{
	CHECKIF(device == NULL) {
		LOG_ERR("Arg invall");
		return -EINVAL;
	}

	uint8_t tmp = device->snd_pkt_id;

	if (tmp == 15) {
		tmp = 0;
	} else {
		tmp++;
	}

	device->snd_pkt_id = tmp;
	return device->snd_pkt_id;
}
