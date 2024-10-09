//
// Created by milho on 8/2/24.
//

#include <app/drivers/lora_tcp.h>
#include "lora_tcp_device.h"
#include "lora_tcp_net.h"

#include <zephyr/drivers/lora.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/crc.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>

LOG_MODULE_REGISTER(lora_tcp);
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

static struct {
	bool is_init;
} self = {
	.is_init = false,
};
/**
 * @brief Driver initialization function
 *
 * @param dev_id ID of the device to be used on P2P communication
 * @return 0 for OK, -X otherwise
 */
int lora_tcp_init(const struct device *dev, uint8_t dev_id, void *cb)
{
	if (self.is_init) {
		return 0;
	}

	CHECKIF(dev == NULL) {
		LOG_ERR("dev is NULL");
		return -ENODEV;
	}

	CHECKIF(cb == NULL) {
		LOG_ERR("Invalid callback pointer");
		return -EINVAL;
	}

	lora_tcp_device_self_set(dev_id);

	lora_tcp_net_init(dev);

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
int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len, uint8_t *rsp,
		  size_t *rsp_len)
{
	struct lora_tcp_device* dev = lora_tcp_device_get_by_id(dest_id);
	if (!dev) {
		LOG_ERR("Device not found");
		return -ENODEV;
	}

	struct lora_tcp_packet* pkt = &dev->packet;

	pkt->header.pkt_id += 1;
	pkt->header.is_ack = false;
	pkt->header.crc = crc32_ieee(data, data_len);

	memcpy(pkt->data, data, data_len);
	pkt->data_len = data_len;

	lora_tcp_net_send(pkt, rsp, rsp_len);
	return 0;
}

int lora_tcp_register(const uint8_t id)
{
	return lora_tcp_device_register(id);
}

int lora_tcp_unregister(const uint8_t id)
{
	return lora_tcp_device_unregister(id);
}

/* --- Module Shell functions --- */

#ifdef CONFIG_LORA_TCP_SHELL

/**
 * @brief Shell command to test lora_tcp functionality
 */
static int control_ping(const struct shell *sh, size_t argc, char **argv)
{
	char ping[] = "ping";

	lora_tcp_send(1, ping, strlen(ping), NULL, NULL);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lora_tcp_sub,
			       SHELL_CMD(ping, NULL, "ping the other device", control_ping),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(lora_tcp, &lora_tcp_sub, "Demo commands", NULL);

#endif /* ifdef CONFIG_LORA_TCP_SHELL */
