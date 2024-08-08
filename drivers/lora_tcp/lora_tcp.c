//
// Created by milho on 8/2/24.
//

#include <app/drivers/lora_tcp.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(lora_tcp);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

struct lora_modem_config config = {
	.frequency = 865100000,
	.bandwidth = BW_125_KHZ,
	.datarate = SF_10,
	.preamble_len = 8,
	.coding_rate = CR_4_5,
	.iq_inverted = false,
	.public_network = false,
	.tx_power = 4,
	.tx = true,
};

struct {
	bool is_init;
	uint8_t id;
} self = {
	.is_init = false,
	.id = 01,
};

static int lora_tcp_build_packet(struct lora_tcp_packet *packet, const uint8_t dest_id,
				 const uint8_t send_id, uint8_t *data, const uint8_t len);

int lora_tcp_init(uint8_t dev_id)
{
	if (self.is_init) {
		return 0;
	}

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready ", lora_dev->name);
		return -1;
	}

	if (lora_config(lora_dev, &config) < 0) {
		LOG_ERR("Lora configuration failed");
		return -EFAULT;
	}

	self.id = dev_id;

	return 0;
}

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len)
{
	struct lora_tcp_packet send_packet;
	struct lora_tcp_packet recv_packet;

	lora_tcp_build_packet(&send_packet, dest_id, self.id, data, data_len);

	lora_send(lora_dev, (uint8_t *)&send_packet, sizeof(struct lora_tcp_packet));

	return 0;
}

static int lora_tcp_build_packet(struct lora_tcp_packet *packet, const uint8_t dest_id,
				 const uint8_t send_id, uint8_t *data, const uint8_t len)
{

	if (len > LORA_TCP_DATA_MAX_SIZE) {
		LOG_ERR("Trying to send too much data");
		return -EINVAL;
	}

	memcpy(packet->encrypted.data, data, len);
	packet->encrypted.data_len = len;

	packet->encrypted.destination_ack = 1000; // TODO random number
	packet->encrypted.sender_ack = 0;
	packet->encrypted.flags = LORA_TCP_FLAG_REQUEST;

	packet->header.destination_id = dest_id;
	packet->header.sender_id = send_id;
	packet->header.crc = -1; // TODO Calculate CRC (full packet, including data)

	return 0;
};
