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

enum lora_tcp_transaction_status {
	LORA_TCP_TRANSACTION_STATUS_IDLE,
	LORA_TCP_TRANSACTION_STATUS_OK,
	LORA_TCP_TRANSACTION_STATUS_FAIL,
};

struct lora_tcp_core_transaction {
	uint8_t count;
	enum lora_tcp_transaction_status status;
	uint8_t *rsp;
	size_t *rsp_len;
	struct lora_tcp_device *device;
	struct lora_tcp_packet packet;
};

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

static void lora_send_timer(struct k_timer *timer);

K_THREAD_DEFINE(recv_thr, CONFIG_LORA_TCP_RECV_THREAD_STACK_SIZE, lora_recv_thread, NULL, NULL,
		NULL, CONFIG_LORA_TCP_RECV_THREAD_PRIORITY, 0, 0);

K_MSGQ_DEFINE(recv_msgq, sizeof(struct lora_tcp_packet), 10, 1);

K_TIMER_DEFINE(lora_send_timer_struct, lora_send_timer, NULL);

K_SEM_DEFINE(lora_send_sem, 1, 1);

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
	lora_tcp_core_cb cb;
	struct lora_tcp_core_transaction transation;
} self = {
	.is_init = false,
	.device = NULL,
	.lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE),
	.cb = NULL,
};

/* --- Public functions --- */

int lora_tcp_core_init(uint8_t dev_id, uint8_t dev_key_id, lora_tcp_core_cb callback)
{

	if (!device_is_ready(self.lora_dev)) {
		LOG_ERR("%s Device not ready ", self.lora_dev->name);
		return -1;
	}

	self.cb = callback;

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

int lora_tcp_core_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len, uint8_t *rsp,
		       size_t *rsp_len)
{
	LOG_ERR("START");
	CHECKIF(data_len > CONFIG_LORA_TCP_DATA_MAX_SIZE) {
		LOG_ERR("Data too long");
		return -EINVAL;
	};

	LOG_ERR("START");
	struct lora_tcp_packet_header *header = &self.transation.packet.header;

	struct lora_tcp_device *device = lora_tcp_device_get_by_id(dest_id);
	if (device == NULL) {
		LOG_WRN("Device not found");
		return -ENODEV;
	}
	LOG_ERR("START");

	device->snd_pkt_id++;
	self.transation.device = device;
	self.transation.status = LORA_TCP_TRANSACTION_STATUS_IDLE;

	header->pkt_id = device->snd_pkt_id;
	header->sender_id = self.device->id;
	header->destination_id = dest_id;
	header->is_ack = false;
	header->status = LORA_TCP_PACKET_STATUS_OK;
	header->crc = crc32_ieee(data, data_len);

	size_t len = data_len;

	memcpy(self.transation.packet.data.buffer, data, data_len);
	self.transation.packet.data.len = data_len;

	self.transation.rsp = rsp;
	self.transation.rsp_len = rsp_len;
	LOG_ERR("START");

	k_timer_start(&lora_send_timer_struct, K_NO_WAIT, K_SECONDS(5));
	k_sem_take(&lora_send_sem, K_NO_WAIT);
	LOG_ERR("START");

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
	struct lora_tcp_packet send_packet = {
		.header = {.is_ack = true, .sender_id = self.device->id}};
	size_t tmp_size;

	struct lora_tcp_device *conn_device;

	enum lora_tcp_conn_result conn_result;

	while (1) {
		k_msgq_get(&recv_msgq, &recv_packet, K_FOREVER);

		const uint32_t calc_crc = crc32_ieee(recv_packet.data.buffer, recv_packet.data.len);
		if (calc_crc != recv_packet.header.crc) {
			LOG_INF("wrong crc16 | expected: %d | recv_packet had: %d", calc_crc,
				recv_packet.header.crc);
			return;
		}

		LOG_INF("Message received");

		if (recv_packet.header.is_ack &&
		    recv_packet.header.sender_id == self.transation.device->id) {

			self.transation.status = recv_packet.header.status
							 ? LORA_TCP_TRANSACTION_STATUS_OK
							 : LORA_TCP_TRANSACTION_STATUS_FAIL;

			if (self.transation.rsp != NULL && self.transation.rsp_len != NULL) {
				*self.transation.rsp_len = recv_packet.data.len;
				memcpy(self.transation.rsp, recv_packet.data.buffer,
				       recv_packet.data.len);
			}

			k_sem_give(&lora_send_sem);

			self.transation.count = 0;
			return;
		}

		lora_tcp_conn_resolve(&recv_packet, &conn_result);

		switch (conn_result) {
		case LORA_TCP_CONN_RESULT_OK:
			tmp_size = CONFIG_LORA_TCP_DATA_MAX_SIZE;

			self.cb(recv_packet.data.buffer, recv_packet.data.len,
				send_packet.data.buffer, &tmp_size);

			send_packet.data.len = tmp_size;

			send_packet.header.status = LORA_TCP_PACKET_STATUS_OK;
			lora_tcp_conn_save_packet(&send_packet);
			break;
		case LORA_TCP_CONN_RESULT_SEND_OLD:
			send_packet.header.status = LORA_TCP_PACKET_STATUS_OK;
			lora_tcp_conn_copy_old_packet(&send_packet);
			break;
		case LORA_TCP_CONN_RESULT_SEND_CONNECTED:
			send_packet.header.status = LORA_TCP_PACKET_STATUS_OK;
			lora_tcp_conn_copy_connected_packet(&send_packet);
			break;

		case LORA_TCP_CONN_RESULT_SEND_REFUSED:
			send_packet.header.status = LORA_TCP_PACKET_STATUS_REFUSED;
			send_packet.data.len = 0;
			break;

		case _LORA_TCP_CONN_RESULT_MAXX:
			LOG_ERR("Shouldn't get here | F");
			break;
		}

		send_packet.header.destination_id = recv_packet.header.sender_id;

		send_packet.header.crc = crc32_ieee(send_packet.data.buffer, send_packet.data.len);

		simple_send(&send_packet);
	}
	return;
}

static void lora_send_timer(struct k_timer *timer)
{
	self.transation.count++;

	simple_send(&self.transation.packet);

	if (self.transation.count > 3) {
		self.transation.count = 0;

		self.transation.status = LORA_TCP_TRANSACTION_STATUS_FAIL;

		k_sem_give(&lora_send_sem);
	}
}
