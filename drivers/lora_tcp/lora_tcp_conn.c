
#include "lora_tcp_conn.h"

#include <zephyr/sys/check.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(lora_tcp_conn);

struct lora_tcp_conn_connection {
	struct lora_tcp_device *device;
	bool is_connected;
};

static struct {
	struct lora_tcp_conn_connection connections[2];
	struct lora_tcp_conn_connection *connected;
	struct lora_tcp_conn_connection *old;
	bool next;
} self = {
	.connected = NULL,
	.old = NULL,
	.next = false,
};

int lora_tcp_conn_start(uint8_t id)
{
	if (self.connected != NULL) {
		return -EBUSY;
	}

	struct lora_tcp_device *dev = lora_tcp_device_get_by_id(id);
	if (dev == NULL) {
		return -ENODEV;
	}

	self.connected = &self.connections[self.next];
	self.connected->is_connected = true;
	self.connected->device = dev;
	self.next = !self.next;

	return 0;
}

int lora_tcp_conn_end(void)
{
	if (self.connected == NULL) {
		return -ENODEV;
	}

	self.connected->is_connected = false;
	self.old = self.connected;
	self.connected = NULL;

	return 0;
}

int lora_tcp_conn_get_connected(struct lora_tcp_device **dev)
{
	CHECKIF(dev == NULL) {
		return -EINVAL;
	}

	if (self.connected != NULL) {
		*dev = self.connected->device;
	}

	return -ENODEV;
}

int lora_tcp_conn_get_old(struct lora_tcp_device **id)
{
	CHECKIF(id == NULL) {
		return -EINVAL;
	}

	if (self.old != NULL) {
		*id = self.old->device;
	};

	return -ENODEV;
}

// Might need to use this thing
int lora_tcp_conn_find(uint8_t id, struct lora_tcp_conn_connection **conn)
{
	CHECKIF(conn == NULL) {
		return -EINVAL;
	};

	for (size_t i = 0; i < 2; i++) {
		if (self.connections[i].device->id == id) {
			*conn = &self.connections[i];
		}
	}

	*conn = NULL;

	return 0;
}
