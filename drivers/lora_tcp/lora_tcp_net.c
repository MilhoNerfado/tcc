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

LOG_MODULE_REGISTER(lora_tcp_net);

#define CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE (1 * 1024)

K_MSGQ_DEFINE(ack_msgq, sizeof(struct lora_tcp_packet), 10, 4);

K_MSGQ_DEFINE(send_msgq, sizeof(struct lora_tcp_packet), 10, 4);
K_MSGQ_DEFINE(recv_msgq, sizeof(struct lora_tcp_packet), 10, 4);

static struct lora_modem_config config;
static const struct device *lora_dev;
static lora_tcp_cb callback;

static void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
                    int8_t snr);

int lora_tcp_net_init(const struct device *dev)
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
	struct lora_tcp_packet send_packet;

	LOG_INF("Received data from device | data_len: %u", size);

	LOG_HEXDUMP_INF(data, size, "Received");

	lora_tcp_packet_unpack(data, size, &packet);

	LOG_INF("send_id: %u | dest_id: %u | crc: %u | pkt_id: %u",
	        packet.header.sender_id, packet.header.destination_id, packet.header.crc,
	        packet.header.pkt_id);

	if (packet.header.destination_id != lora_tcp_device_self_get()->id) {
		return;
	}

	if (packet.header.crc != crc32_ieee(packet.data, packet.data_len)) {
		LOG_ERR("CRC mismatch | expected: %d got: %d",
		        crc32_ieee(packet.data, packet.data_len), packet.header.crc);
		return;
	}

	if (packet.header.is_ack) {
		LOG_WRN("Received ACK");
		k_msgq_put(&ack_msgq, &packet, K_FOREVER);
		return;
	}

	k_msgq_put(&recv_msgq, &packet, K_FOREVER);
}

int lora_tcp_net_send(struct lora_tcp_packet *pkt, uint8_t *rsp, size_t *rsp_len)
{
	CHECKIF(pkt == NULL) {
		LOG_ERR("Received NULL packet");
		return -EFAULT;
	}

	uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
	size_t buffer_len;

	struct lora_tcp_packet packet;

	LOG_INF("Sending\n");

	lora_tcp_packet_build(pkt, buffer, &buffer_len);

	k_msgq_purge(&ack_msgq);

	for (int i = 0; i < 3; ++i) {
		lora_recv_async(lora_dev, NULL);
		config.tx = true;
		lora_config(lora_dev, &config);

		int err = lora_send(lora_dev, buffer, buffer_len);
		if (err != 0) {
			LOG_WRN("Failed to send packet | err: %d", err);
		}

		LOG_INF("[send_packet] Sent %d bytes\n", buffer_len);

		config.tx = false;
		lora_config(lora_dev, &config);
		lora_recv_async(lora_dev, recv_cb);

		if (k_msgq_get(&ack_msgq, &packet, K_SECONDS(5)) == 0) {

			LOG_WRN("Received packet from device");

			if (packet.data_len == 0 || rsp == NULL || rsp_len == NULL) {
				return 0;
			}

			memcpy(rsp, packet.data, packet.data_len);
			*rsp_len = packet.data_len;

			LOG_HEXDUMP_INF(rsp, packet.data_len, "ACK Data Saved");

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

	while (true) {
		if (k_msgq_get(&send_msgq, &packet, K_FOREVER) != 0) {
			LOG_ERR("Failed to receive packet from device");
		}

		lora_tcp_packet_build(&packet, buffer, &buffer_len);

		lora_recv_async(lora_dev, NULL);
		config.tx = true;
		lora_config(lora_dev, &config);

		int err = lora_send(lora_dev, buffer, buffer_len);
		if (err != 0) {
			LOG_WRN("Failed to send packet | err: %d", err);
		}

		LOG_INF("[send_packet] Sent %d bytes\n", buffer_len);

		config.tx = false;
		lora_config(lora_dev, &config);
		lora_recv_async(lora_dev, recv_cb);
	}
}

void recv_thread(void *, void *, void *)
{
	struct lora_tcp_packet packet;

	while (true) {
		if (k_msgq_get(&recv_msgq, &packet, K_FOREVER) != 0) {
			LOG_ERR("Failed to receive packet from device");
		}

		packet.header.is_ack = true;
		packet.header.destination_id =
			packet.header.destination_id ^ packet.header.sender_id;
		packet.header.sender_id = packet.header.destination_id ^ packet.header.sender_id;
		packet.header.destination_id =
			packet.header.destination_id ^ packet.header.sender_id;

		k_msgq_put(&send_msgq, &packet, K_FOREVER);
		LOG_INF("Sent to send queue");
	}
}

K_THREAD_DEFINE(send_tid, CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE, send_thread, NULL, NULL, NULL, 5,
                0, 0);

K_THREAD_DEFINE(recv_tid, CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE, send_thread, NULL, NULL, NULL, 5,
                0, 0);