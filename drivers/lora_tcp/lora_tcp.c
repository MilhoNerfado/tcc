//
// Created by milho on 8/2/24.
//

#include <app/drivers/lora_tcp.h>
#include "lora_tcp_packet.h"
#include "lora_tcp_conn.h"

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

static struct {
	bool is_init;
	struct lora_tcp_device *device;
	const struct device *const lora_dev;
	struct k_fifo *fifo;
} self = {
	.is_init = false,
	.device = NULL,
	.lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE),
	.fifo = NULL,
};

/**
 * @brief Callback for receiving messages from lora configured band
 *
 * @param dev lora device used on received
 * @param data pointer to data received
 * @param size length of bytes received
 * @param rssi rssi value of message received
 * @param snr snr value of the message received
 */
static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr);

/**
 * @brief Thread for receiving data directly from the the lora_receive_cb using a message queue as
 * medium.
 */
static void lora_recv_thread(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(recv_thr, CONFIG_LORA_TCP_RECV_THREAD_STACK_SIZE, lora_recv_thread, NULL, NULL,
		NULL, CONFIG_LORA_TCP_RECV_THREAD_PRIORITY, 0, 0);

K_MSGQ_DEFINE(recv_msgq, sizeof(struct lora_tcp_packet), 10, 1);

/**
 * @brief Driver initialization function
 *
 * @param dev_id ID of the device to be used on P2P communication
 * @param fifo pointer to a FIFO to store received messages
 * @return 0 for OK, -X otherwise
 */
int lora_tcp_init(uint8_t dev_id, uint8_t dev_key_id, struct k_fifo *fifo)
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

	lora_tcp_device_self_set(dev_id, dev_key_id);

	self.device = lora_tcp_device_self_get();

	self.fifo = fifo;

	self.is_init = true;

	LOG_WRN("Lora TCP Started");

	return 0;
}

/**
 * @brief Package and send data using lora tcp protocol
 *
 * @param dest_id ID of the receiver device
 * @param data pointer to an buffer containing data to be sent
 * @param data_len lenght of data to be sent
 * @return 0 for OK, -X otherwise
 */
int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len)
{
	CHECKIF(data_len > CONFIG_LORA_TCP_DATA_MAX_SIZE) {
		LOG_ERR("Data too long");
		return -1;
	};

	uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
	size_t buffer_len;
	const uint32_t crc = crc32_ieee(data, data_len);

	struct lora_tcp_packet packet = {
		.header.sender_id = self.device->id,
		.header.destination_id = dest_id,
		.header.crc = crc,
		.header.is_sync = false,
		.header.is_ack = false,
		.header.status = LORA_TCP_PACKET_STATUS_OK,
		.data.len = data_len,
	};

	memset(packet.data.buffer, 0, CONFIG_LORA_TCP_DATA_MAX_SIZE);
	memcpy(packet.data.buffer, data, data_len);

	lora_tcp_packet_build(&packet, buffer, &buffer_len);

	lora_recv_async(self.lora_dev, NULL);
	lora_comm_config.tx = true;
	lora_config(self.lora_dev, &lora_comm_config);

	lora_send(self.lora_dev, buffer, buffer_len);

	lora_comm_config.tx = false;
	lora_config(self.lora_dev, &lora_comm_config);
	lora_recv_async(self.lora_dev, lora_receive_cb);

	return 0;
}

/* --- Local functions --- */

static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr)
{
	struct lora_tcp_packet packet = {0};

	LOG_INF("CB | Size: %d", size);

	if (size < sizeof(struct lora_tcp_packet_header)) {
		LOG_INF("pkt too small");
		return;
	}

	lora_tcp_packet_unpack(data, size, &packet);

	if (packet.header.destination_id != self.device->id) {
		LOG_INF("destination ID not self ID | dest id: %d", packet.header.destination_id);
		return;
	}

	/* TODO: check errors from msg queue */
	k_msgq_put(&recv_msgq, &packet, K_NO_WAIT);

	LOG_WRN("Received msg:");
	LOG_HEXDUMP_WRN(packet.data.buffer, packet.data.len, " ");

	return;
}

static void lora_recv_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct lora_tcp_packet packet;

	struct lora_tcp_device *conn_device;

	while (1) {
		k_msgq_get(&recv_msgq, &packet, K_FOREVER);

		const uint32_t calc_crc = crc32_ieee(packet.data.buffer, packet.data.len);
		if (calc_crc != packet.header.crc) {
			LOG_INF("wrong crc16 | expected: %d | packet had: %d", calc_crc,
				packet.header.crc);
			return;
		}

		LOG_INF("Message received");

		if (packet.header.is_ack) {
			// TODO: Add sync packet support later
			return;
		}

		conn_device = lora_tcp_device_get_by_id(packet.header.sender_id);

		int err = lora_tcp_conn_start(conn_device);

		if (err == -EBUSY) {
			if (lora_tcp_conn_get_connected() == conn_device) {
				// TODO: Send connected packet
				return;
			}
			if (lora_tcp_conn_get_old() == conn_device) {
				// TODO: Send old packet
				return;
			}
			// TODO: SEND device busy status packet
		}

		if (err == -ENODEV) {
			// TODO: SEND refused status packet
		}

		// TODO: Run callback function
		//
		// TODO: Fill packet to send

		// TODO: Send ACK (with data)
	}
	return;
}

/* --- Module Shell functions --- */

#ifdef CONFIG_LORA_TCP_SHELL

/**
 * @brief Shell command to test lora_tcp functionality
 */
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
