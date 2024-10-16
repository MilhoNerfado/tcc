//
// Created by milho on 10/2/24.
//

#include "lora_tcp_net.h"

#include "lora_tcp_device.h"
#include "lora_tcp_packet.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(lora_tcp_net, CONFIG_LORA_TCP_NET_LOG_LEVEL);

#define CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE (1 * 512)

K_MSGQ_DEFINE(ack_msgq, sizeof(struct lora_tcp_packet), 10, 4);

K_MSGQ_DEFINE(send_msgq, sizeof(struct lora_tcp_packet), 10, 4);
K_MSGQ_DEFINE(recv_msgq, sizeof(struct lora_tcp_packet), 10, 4);

static struct lora_modem_config config;
static const struct device *lora_dev;
static lora_tcp_cb user_callback;

static void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
		    int8_t snr);

int lora_tcp_net_init(const struct device *dev, void *callback)
{
	if (dev == NULL) {
		LOG_ERR("Device is NULL");
		return -ENODEV;
	}

	lora_dev = dev;

	user_callback = callback;

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

	if (lora_config(lora_dev, &config)) {
		LOG_ERR("Failed to configure lora device");
		return -EIO;
	}

	lora_recv_async(lora_dev, recv_cb);

	return 0;
}

static void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
		    int8_t snr)
{
	struct lora_tcp_packet packet;

	LOG_HEXDUMP_ERR(data, size, "[recv_cb] Received");

	lora_tcp_packet_unpack(data, size, &packet);

	if (packet.header.destination_id != lora_tcp_device_self_get()->id) {
		LOG_INF("Not self id");
		LOG_INF("self id: %u | packet dest id: %u", lora_tcp_device_self_get()->id,
			packet.header.destination_id);
		return;
	}

	if (packet.header.crc != crc32_ieee(packet.data, packet.data_len)) {
		LOG_ERR("CRC mismatch | expected: %d got: %d",
			crc32_ieee(packet.data, packet.data_len), packet.header.crc);
		return;
	}

	LOG_INF("[recv_cb] Received Packet | crc: %u | data_len: %u | send_id: %u | dest_id: %u | "
		"packet_id: "
		"%u | "
		"is_ack: %u",
		packet.header.crc, packet.data_len, packet.header.sender_id,
		packet.header.destination_id, packet.header.pkt_id, packet.header.is_ack);

	k_msgq_put(&recv_msgq, &packet, K_FOREVER);

	LOG_INF("Send recv msg to recv_msgq");
}

int lora_tcp_net_send(struct lora_tcp_packet *pkt, uint8_t *rsp, size_t *rsp_len)
{
	CHECKIF(pkt == NULL) {
		LOG_ERR("Received NULL packet");
		return -EFAULT;
	}

	uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
	size_t buffer_len;

	struct lora_tcp_packet ack_packet;

	LOG_INF("Sending\n");

	lora_tcp_packet_build(pkt, buffer, &buffer_len);

	// k_msgq_purge(&ack_msgq);

	for (int i = 0; i < 3; ++i) {
		lora_recv_async(lora_dev, NULL);
		config.tx = true;
		lora_config(lora_dev, &config);

		LOG_HEXDUMP_ERR(buffer, buffer_len, "[lora_tcp_net_send] Sending");

		int err = lora_send(lora_dev, buffer, buffer_len);
		if (err != 0) {
			LOG_WRN("Failed to send packet | err: %d", err);
		}

		LOG_INF("[send_packet] Sent %d bytes\n", buffer_len);

		config.tx = false;
		lora_config(lora_dev, &config);
		lora_recv_async(lora_dev, recv_cb);

		if (k_msgq_get(&ack_msgq, &ack_packet, K_SECONDS(5)) == 0) {

			LOG_WRN("Received packet from device");

			if (rsp == NULL || rsp_len == NULL) {
				LOG_WRN("No response buffer pointer");
				return 0;
			}

			memcpy(rsp, ack_packet.data, ack_packet.data_len);
			*rsp_len = ack_packet.data_len;

			LOG_HEXDUMP_INF(rsp, *rsp_len, "ACK Data Saved");

			return 0;
		}
	}

	return -1;
}

void send_thread(void *, void *, void *)
{
	struct lora_tcp_packet packet;
	uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
	size_t buffer_len;

	LOG_WRN("Thread started");

	while (true) {
		if (k_msgq_get(&send_msgq, &packet, K_FOREVER) != 0) {
			LOG_ERR("Failed to receive packet from device");
		}

		LOG_INF("[send_thread] Received packet from device");
		LOG_INF("[send_thread] send_id: %u | dest_id: %u | crc: %u | pkt_id: %u is_ack: %s",
			packet.header.sender_id, packet.header.destination_id, packet.header.crc,
			packet.header.pkt_id, packet.header.is_ack ? "true" : "false");

		lora_tcp_packet_build(&packet, buffer, &buffer_len);

		lora_recv_async(lora_dev, NULL);
		config.tx = true;
		lora_config(lora_dev, &config);

		LOG_HEXDUMP_ERR(buffer, buffer_len, "[send_thread] Sending");
		int err = lora_send(lora_dev, buffer, buffer_len);
		if (err != 0) {
			LOG_WRN("Failed to send packet | err: %d", err);
		}

		config.tx = false;
		lora_config(lora_dev, &config);
		lora_recv_async(lora_dev, recv_cb);
	}
}

void recv_thread(void *, void *, void *)
{
	struct lora_tcp_packet packet;

	LOG_WRN("Thread started");

	while (true) {
		if (k_msgq_get(&recv_msgq, &packet, K_FOREVER) != 0) {
			LOG_ERR("Failed to receive packet from device");
		}

		LOG_INF("[recv_thread] Received Packet | crc: %u | data_len: %u | send_id: %u | "
			"dest_id: %u | "
			"packet_id: %u | "
			"is_ack: %u",
			packet.header.crc, packet.data_len, packet.header.sender_id,
			packet.header.destination_id, packet.header.pkt_id, packet.header.is_ack);

		struct lora_tcp_device *device = lora_tcp_device_get_by_id(packet.header.sender_id);

		if (device == NULL) {
			LOG_ERR("Received message from unknown device");
			continue;
		}

		if (packet.header.is_ack) {
			LOG_WRN("Received ACK");
			k_msgq_put(&ack_msgq, &packet, K_FOREVER);
			LOG_WRN("Received ACK");
			continue;
		}

		if (packet.header.pkt_id == device->recv_packet.header.pkt_id) {
			LOG_WRN("Same pkg");
			k_msgq_put(&send_msgq, &device->recv_packet, K_FOREVER);
			LOG_WRN("Same pkg");
			continue;
		}

		device->recv_packet.header.pkt_id = packet.header.pkt_id;

		// user callback
		if (user_callback != NULL) {
			memset(device->recv_packet.data, 0, CONFIG_LORA_TCP_DATA_MAX_SIZE);
			device->recv_packet.data_len = CONFIG_LORA_TCP_DATA_MAX_SIZE;

			user_callback(packet.header.sender_id, packet.data, packet.data_len,
				      device->recv_packet.data, &device->recv_packet.data_len);
		}

		device->recv_packet.header.crc =
			crc32_ieee(device->recv_packet.data, device->recv_packet.data_len);

		device->recv_packet.header.is_ack = true;
		device->recv_packet.header.destination_id = packet.header.sender_id;
		device->recv_packet.header.sender_id = packet.header.destination_id;

		LOG_INF("Sent to send queue");
		k_msgq_put(&send_msgq, &device->recv_packet, K_FOREVER);
		LOG_INF("Sent to send queue");
	}
}

K_THREAD_DEFINE(send_tid, CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE, send_thread, NULL, NULL, NULL, 5,
		0, 0);

K_THREAD_DEFINE(recv_tid, CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE, recv_thread, NULL, NULL, NULL, 5,
		0, 0);