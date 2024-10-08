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

LOG_MODULE_REGISTER(lora_tcp_net);

#define CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE (1 * 1024)

static struct lora_modem_config config;
static const struct device *lora_dev;

static uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE];
static size_t buffer_len;

static void recv_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
		    int8_t snr);

static void send_packet();

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
	LOG_INF("Received data from device | data_len: %u", size);

	LOG_HEXDUMP_INF(data, size, "Received");
}

int lora_tcp_net_send(struct lora_tcp_packet *pkt)
{
	CHECKIF(pkt == NULL) {
		LOG_ERR("Received NULL packet");
		return -EFAULT;
	}

	LOG_INF("Sending\n");

	lora_tcp_packet_build(pkt, buffer, &buffer_len);

	send_packet();

	return 0;
}

static void send_packet()
{
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

void inner_thread(void *, void *, void *)
{
	k_sleep(K_MSEC(5000));
	LOG_INF("threading\n");
	k_sleep(K_FOREVER);
}

/*K_THREAD_DEFINE(inner_tid, CONFIG_LORA_TCP_NET_THREAD_STACK_SIZE, inner_thread, NULL, NULL, NULL,
   5, 0, 0);*/
