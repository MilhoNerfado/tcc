//
// Created by milho on 8/2/24.
//

#include <app/drivers/lora_tcp.h>
#include "lora_tcp_packet.h"

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/crc.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(lora_tcp);
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

struct lora_modem_config lora_comm_config = {
	.frequency = 865100000,
	.bandwidth = BW_125_KHZ,
	.datarate = SF_10,
	.preamble_len = 8,
	.coding_rate = CR_4_5,
	.iq_inverted = false,
	.public_network = false,
	.tx_power = 14,
	.tx = false,
};

struct {
	bool is_init;
	uint8_t id;
	const struct device *const lora_dev;
	struct k_fifo *fifo;
} self = {
	.is_init = false,
	.id = 0,
	.lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE),
	.fifo = NULL,
};

static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr);

int lora_tcp_init(uint8_t dev_id, struct k_fifo *fifo)
{
	if (self.is_init) {
		return 0;
	}

	if (fifo == NULL) {
		return -EINVAL;
	}

	if (!device_is_ready(self.lora_dev)) {
		LOG_ERR("%s Device not ready ", self.lora_dev->name);
		return -1;
	}

	if (lora_config(self.lora_dev, &lora_comm_config) < 0) {
		LOG_ERR("Lora configuration failed");
		return -EFAULT;
	}

	lora_recv_async(self.lora_dev, lora_receive_cb);

	self.id = dev_id;
	self.fifo = fifo;

	self.is_init = true;

	LOG_WRN("Lora TCP Started");

	return 0;
}

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len)
{
	CHECKIF(data_len > CONFIG_LORA_TCP_DATA_MAX_SIZE) {
		LOG_ERR("Data too long");
		return -1;
	};

	uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
	size_t buffer_len;
	const uint32_t crc = crc32_ieee(data, data_len);

	lora_tcp_packet_build(dest_id, self.id, crc, data, data_len, buffer, &buffer_len);

	lora_recv_async(self.lora_dev, NULL);
	lora_comm_config.tx = true;
	lora_config(self.lora_dev, &lora_comm_config);

	lora_send(self.lora_dev, buffer, buffer_len);

	lora_comm_config.tx = false;
	lora_config(self.lora_dev, &lora_comm_config);
	lora_recv_async(self.lora_dev, lora_receive_cb);

	return 0;
}

static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr)
{
	struct lora_tcp_packet packet = {0};

	LOG_INF("CB | Size: %d", size);

	if (size < sizeof(struct lora_tcp_packet_header)) {
		LOG_INF("pkt too small");
		return;
	}

	/*TODO: UNPACK*/
	lora_tcp_packet_unpack(data, size, &packet);

	/* TODO: Check ID */
	if (packet.header.destination_id != self.id) {
		LOG_INF("wrong id | dest id: %d", packet.header.destination_id);
		return;
	}

	/* TODO: Check CRC */
	const uint32_t calc_crc = crc32_ieee(packet.data.buffer, packet.data.len);
	// if (calc_crc != packet.header.crc) {
	// 	return;
	// }

	/* TODO: All OK, send to fifo */

	LOG_WRN("Received msg:");
	LOG_HEXDUMP_WRN(packet.data.buffer, packet.data.len, " ");

	return;
}

/* --- */

#ifdef CONFIG_LORA_TCP_SHELL

static int control_ping(const struct shell *sh, size_t argc, char **argv)
{
	char ping[] = "ping";
	lora_tcp_send(1, ping, strlen(ping));
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lora_tcp_sub,
			       SHELL_CMD(ping, NULL, "ping the other device", control_ping),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(lora_tcp, &lora_tcp_sub, "Demo commands", NULL);

#endif /* ifdef CONFIG_LORA_TCP_SHELL */
