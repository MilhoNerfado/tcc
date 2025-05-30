#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/zbus/zbus.h>
#include <sys/types.h>

#include "lora_udp.h"
#include "xtea.h"

#include "zephyr/logging/log.h"
#include "zephyr/logging/log_core.h"
#include "zephyr/sys/crc.h"

LOG_MODULE_REGISTER(radio_lib, LOG_LEVEL_DBG);

ZBUS_CHAN_DECLARE(output_chan);

static const struct device *lora = DEVICE_DT_GET(DT_ALIAS(lora0));

static struct lora_modem_config config = {
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

static const uint32_t key[4] = {11666574, 3905216703, 702153731, 1592004906};

void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr)
{
	ARG_UNUSED(dev);
	LOG_DBG("New lora message | rssi: %d | snr: %d", rssi, snr);
	LOG_HEXDUMP_DBG(data, size, " Data:");

	uint32_t deciphered[16] = {0};
	uint32_t crc = 0;
	struct lora_udp_packet *msg;
	struct chan_out *outs;

	if (size > sizeof(deciphered) || data == NULL) {
		LOG_DBG("Ignored size too big\n");
		return;
	}

	memset(deciphered, 0, sizeof(deciphered));
	memcpy(deciphered, data, size);

	/* for (int i = 0; i < 16; i += 2) { */
	/* 	decipher(64, deciphered + (i), key); */
	/* } */

	LOG_HEXDUMP_DBG(data, size, " Deciphered:");

	msg = (struct lora_udp_packet *)&deciphered;
	outs = (struct chan_out *)&msg->data;

	if (msg == NULL) {
		LOG_INF("msg == NULL\n");
	}
	if (msg->header.data_len) {
		LOG_DBG("data_len %lu\n", msg->header.data_len);
	}
	if (outs == NULL) {
		LOG_DBG("outs == NULL\n");
	}

	if (msg->header.destination_id != 1) {
		LOG_DBG("Invalid destination_id | got: %d | expected: %d \n",
			msg->header.destination_id, 1);
		return;
	}

	if (msg->header.sender_id != 1) {
		LOG_DBG("Invalid sender_id | got: %d | expected: %d\n", msg->header.sender_id, 1);
		return;
	}

	crc = crc32_ieee((uint8_t *)outs, msg->header.data_len);
	if (crc != msg->header.crc) {
		LOG_DBG("Invalid crc | got: %d | expected: %d\n", crc, msg->header.crc);
		return;
	}

	LOG_INF("output0: %d output1: %d\n", outs->outputs[0], outs->outputs[1]);

	zbus_chan_pub(&output_chan, outs, K_MSEC(200));
}

void radio_handler(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	struct chan_out chan_msg;
	struct lora_udp_packet pkt;
	uint32_t buffer[16];

	pkt.header.data_len = sizeof(chan_msg);

	lora_config(lora, &config);

	lora_recv_async(lora, recv_cb);

	while (true) {
		k_msleep(3000);

		lora_recv_async(lora, NULL);
		config.tx = true;

		lora_config(lora, &config);

		zbus_chan_read(&output_chan, &chan_msg, K_MSEC(200));

		pkt.header.crc = crc32_ieee((uint8_t *)&chan_msg, pkt.header.data_len);
		pkt.header.destination_id = 1;
		pkt.header.sender_id = 1;

		memcpy((uint8_t *)buffer, &pkt.header, sizeof(struct lora_udp_header));
		memcpy(((uint8_t *)buffer) + sizeof(struct lora_udp_header), &chan_msg,
		       pkt.header.data_len);

		/* for (int i = 0; i < 16; i += 2) { */
		/* 	encipher(64, buffer + (i), key); */
		/* } */

		LOG_DBG("Sending data\n");

		lora_send(lora, (uint8_t *)buffer,
			  sizeof(struct lora_udp_header) + pkt.header.data_len);

		config.tx = false;
		lora_config(lora, &config);

		lora_recv_async(lora, recv_cb);
	}
}
K_THREAD_DEFINE(raido_handler_tid, 1024, radio_handler, NULL, NULL, NULL, 5, 0, 0);
