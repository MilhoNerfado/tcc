#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/zbus/zbus.h>

#include "lora_udp.h"
#include "zephyr/logging/log.h"
#include "zephyr/logging/log_core.h"
#include "zephyr/sys/printk.h"

LOG_MODULE_DECLARE(lora_udp);

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

void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi, int8_t snr)
{
	ARG_UNUSED(dev);
	printf("New lora message | rssi: %d | snr: %d", rssi, snr);
	LOG_HEXDUMP_DBG(data, size, " Data:");

	struct lora_udp_packet *msg = (struct lora_udp_packet *)data;
	struct chan_out *outs = msg->data;

	if (data == NULL || size < sizeof(struct lora_udp_header) || size < msg->header.data_len) {
		LOG_DBG("Ignored lora msg");
		return;
	}

	printf("output0: %b output1: %b", outs->outputs[0], outs->outputs[0]);

	zbus_chan_pub(&output_chan, msg->data, K_MSEC(200));
}

void radio_handler(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	struct chan_out chan_msg;
	struct lora_udp_packet pkt;
	uint8_t buffer[255];

	pkt.header.data_len = sizeof(chan_msg);

	lora_config(lora, &config);

	lora_recv_async(lora, recv_cb);

	while (true) {
		k_msleep(400);
		zbus_chan_read(&output_chan, &chan_msg, K_MSEC(200));

		lora_recv_async(lora, NULL);

		config.tx = true;
		lora_config(lora, &config);

		memcpy(buffer, &pkt.header, sizeof(struct lora_udp_header));
		memcpy(buffer + sizeof(struct lora_udp_header), &chan_msg, pkt.header.data_len);

		printk("Sending data\n");

		lora_send(lora, buffer, sizeof(struct lora_udp_header) + pkt.header.data_len);

		config.tx = false;
		lora_config(lora, &config);
		lora_recv_async(lora, recv_cb);
	}
}
K_THREAD_DEFINE(raido_handler_tid, 1024, radio_handler, NULL, NULL, NULL, 5, 0, 0);
