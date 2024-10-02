
#include "lora_tcp_conn.h"

#include <zephyr/sys/check.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(lora_tcp_conn);

struct lora_tcp_conn_connection {
	struct lora_tcp_device *device;
	struct lora_tcp_packet packet;
	uint8_t pkt_id: 4;
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

void lora_tcp_conn_resolve(struct lora_tcp_packet *recv_packet,
			   enum lora_tcp_conn_result *resolve_result)
{
	struct lora_tcp_packet send_packet;

	struct lora_tcp_device *conn_device;

	conn_device = lora_tcp_device_get_by_id(recv_packet->header.sender_id);

	if (conn_device == NULL) {
		*resolve_result = LORA_TCP_CONN_RESULT_SEND_REFUSED;
		// TODO: SEND refused status packet
		return;
	}

	int err = lora_tcp_conn_start(conn_device, recv_packet->header.pkt_id);

	/* Self already processing another message */
	if (err == -EBUSY) {
		if (lora_tcp_conn_get_old() == conn_device &&
		    lora_tcp_conn_get_old_pkt_id() == recv_packet->header.pkt_id) {
			*resolve_result = LORA_TCP_CONN_RESULT_SEND_OLD;
			// TODO: Send old packet
			return;
		}

		if (lora_tcp_conn_get_connected() == conn_device &&
		    lora_tcp_conn_get_connected_pkt_id() == recv_packet->header.pkt_id) {
			*resolve_result = LORA_TCP_CONN_RESULT_SEND_CONNECTED;
			// TODO: Send connected packet
			return;
		}
	}
	lora_tcp_conn_end();
	lora_tcp_conn_start(conn_device, recv_packet->header.pkt_id);
	*resolve_result = LORA_TCP_CONN_RESULT_OK;
	// TODO: Proceed normaly
}

int lora_tcp_conn_start(struct lora_tcp_device *device, uint8_t pkt_id)
{
	CHECKIF(device == NULL) {
		return -ENODEV;
	}

	if (self.connected != NULL) {
		return -EBUSY;
	}

	self.connected = &self.connections[self.next];
	self.connected->is_connected = true;
	self.connected->device = device;
	self.connected->pkt_id = pkt_id;
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

struct lora_tcp_device *lora_tcp_conn_get_connected(void)
{
	if (self.connected != NULL) {
		return self.connected->device;
	}
	return NULL;
}
int lora_tcp_conn_get_connected_pkt_id(void)
{
	if (self.connected == NULL) {
		return -1;
	}
	return self.connected->pkt_id;
}

int lora_tcp_conn_save_packet(struct lora_tcp_packet *packet)
{
	CHECKIF(packet == NULL) {
		LOG_ERR("Pointer null");
		return -EINVAL;
	}

	if (self.connected == NULL) {
		LOG_ERR("Connection not initialized");
		return -ENOENT;
	}

	memcpy(&self.connected->packet, packet, sizeof(struct lora_tcp_packet));

	return 0;
}

int lora_tcp_conn_copy_connected_packet(struct lora_tcp_packet *packet)
{
	CHECKIF(packet == NULL) {
		LOG_ERR("Pointer null");
		return -EINVAL;
	}

	if (self.connected == NULL) {
		LOG_ERR("Connection not initialized");
		return -ENOENT;
	}

	memcpy(packet, &self.connected->packet, sizeof(struct lora_tcp_packet));

	return 0;
}

struct lora_tcp_device *lora_tcp_conn_get_old(void)
{
	if (self.old != NULL) {
		return self.old->device;
	};

	return NULL;
}

int lora_tcp_conn_get_old_pkt_id(void)
{
	if (self.old == NULL) {
		return -1;
	}
	return self.old->pkt_id;
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

int lora_tcp_conn_copy_old_packet(struct lora_tcp_packet *packet)
{
	CHECKIF(packet == NULL) {
		LOG_ERR("Pointer null");
		return -EINVAL;
	}

	if (self.old == NULL) {
		LOG_ERR("Currently no old packet");
		return -ENOENT;
	}

	memcpy(packet, &self.old->packet, sizeof(struct lora_tcp_packet));

	return 0;
}
