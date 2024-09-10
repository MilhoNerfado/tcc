
#include "lora_tcp_conn.h"

#include <zephyr/sys/check.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(lora_tcp_conn);

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

	self.connected = &self.connections[self.next];
	self.connected->is_connected = true;
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

int lora_tcp_conn_get_connected(uint8_t *id)
{
	CHECKIF(id == NULL) {
		return -EINVAL;
	}

	if (self.connected != NULL) {
		*id = self.connected->device_id;
	}

	return -ENODEV;
}

int lora_tcp_conn_get_old(uint8_t *id)
{
	CHECKIF(id == NULL) {
		return -EINVAL;
	}

	if (self.old != NULL) {
		*id = self.old->device_id;
	};

	return -ENODEV;
}

int lora_tcp_conn_find(uint8_t id, struct lora_tcp_conn_connection **conn)
{
	CHECKIF(conn == NULL) {
		return -EINVAL;
	};

	for (size_t i = 0; i < 2; i++) {
		if (self.connections[i].device_id == id) {
			*conn = &self.connections[i];
		}
	}

	*conn = NULL;

	return 0;
}
