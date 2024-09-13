#include "lora_tcp_core.h"
#include "lora_tcp_conn.h"

#include "lora_tcp_packet.h"

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

LOG_MODULE_REGISTER(lora_tcp_core);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

static int simple_send(struct lora_tcp_packet *packet);

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

} self = {
	.is_init = false,
	.device = NULL,
	.lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE),
};

/* --- Public functions --- */

int lora_tcp_core_init(uint8_t dev_id, uint8_t dev_key_id)
{

	if (!device_is_ready(self.lora_dev)) {
		LOG_ERR("%s Device not ready ", self.lora_dev->name);
		return -1;
	}

	/* Init Lora on correct mode */
	lora_comm_config.tx = false;

	if (lora_config(self.lora_dev, &lora_comm_config) < 0) {
		LOG_ERR("Lora configuration failed");
		return -EFAULT;
	}

	lora_recv_async(self.lora_dev, lora_receive_cb);

	lora_tcp_device_self_set(dev_id, dev_key_id);

	self.device = lora_tcp_device_self_get();
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
int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len, uint8_t *response,
		  uint8_t *response_len, size_t response_size)
{
	CHECKIF(data_len > CONFIG_LORA_TCP_DATA_MAX_SIZE) {
		LOG_ERR("Data too long");
		return -EINVAL;
	};

	CHECKIF(response == NULL && response_size != 0) {
		LOG_ERR("Response buffer expected but not given, response_size: %lu",
			response_size);
		return -EINVAL;
	}

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

	simple_send(&packet);

	return 0;
}

/* --- Local functions --- */

static int simple_send(struct lora_tcp_packet *packet)
{
	uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
	size_t buffer_len;

	lora_tcp_packet_build(packet, buffer, &buffer_len);

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

	struct lora_tcp_packet recv_packet;
	struct lora_tcp_packet send_packet;

	struct lora_tcp_device *conn_device;

	while (1) {
		k_msgq_get(&recv_msgq, &recv_packet, K_FOREVER);

		const uint32_t calc_crc = crc32_ieee(recv_packet.data.buffer, recv_packet.data.len);
		if (calc_crc != recv_packet.header.crc) {
			LOG_INF("wrong crc16 | expected: %d | recv_packet had: %d", calc_crc,
				recv_packet.header.crc);
			return;
		}

		LOG_INF("Message received");

		if (recv_packet.header.is_ack) {
			// TODO: Send Data to sender (mutex??) | If it fails means that no ack was
			// expected, so should send a timeout.

			// TODO: Add sync packet support later
			return;
		}

		lora_tcp_conn_resolve(&recv_packet);

		// TODO: Run callback function
		//
		// TODO: Fill packet to send

		// TODO: Send ACK (with data)
	}
	return;
}
