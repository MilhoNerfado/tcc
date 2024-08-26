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
	.tx_power = 4,
	.tx = true,
};

struct {
	bool is_init;
	uint8_t id;
	const struct device *const lora_dev;
} self = {
	.is_init = false,
	.id = 0,
	.lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE),
};

static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr);

int lora_tcp_init(uint8_t dev_id)
{
	if (self.is_init) {
		return 0;
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

	self.is_init = true;

	LOG_WRN("Lora TCP Started");

	return 0;
}

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len)
{
	int err;

	lora_recv_async(self.lora_dev, NULL);

	lora_send(self.lora_dev, (uint8_t *)data, data_len);

	lora_recv_async(self.lora_dev, lora_receive_cb);

	return 0;
}

static void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
			    int8_t snr)
{
	static int cnt;
	static uint8_t buffer[255];

	ARG_UNUSED(dev);

	printf("AAAAAAAAA");

	memset(buffer, 0, 255);
	memcpy(buffer, data, size);

	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)", buffer, rssi, snr);
}
