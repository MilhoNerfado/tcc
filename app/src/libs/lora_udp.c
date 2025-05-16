#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/zbus/zbus.h>

#include "lora_udp.h"
#include "zephyr/logging/log.h"
#include "zephyr/logging/log_core.h"

LOG_MODULE_DECLARE(lora_udp, LOG_LEVEL_DBG);

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
	LOG_DBG("New lora message | rssi: %d | snr: %d", rssi, snr);
	LOG_HEXDUMP_DBG(data, size, "");

	struct lora_udp_packet *msg = (struct lora_udp_packet *)data;

	if (data == NULL || size < sizeof(struct lora_udp_header) || size < msg->header.data_len) {
		LOG_DBG("Ignored lora msg");
		return;
	}

	zbus_chan_pub(&output_chan, msg->data, K_MSEC(200));
}

void radio_handler(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg0);
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	lora_config(lora, &config);

	lora_recv_async(lora, recv_cb);

	while (true) {
		k_msleep(1000);
		zbus_chan_read(&output_chan, NULL, K_MSEC(200));

		lora_recv_async(lora, NULL);
		lora_send(lora, NULL, 0);
		lora_recv_async(lora, recv_cb);
	}
}
K_THREAD_DEFINE(raido_handler_tid, 1024, radio_handler, NULL, NULL, NULL, 5, 0, 0);
