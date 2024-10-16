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

LOG_MODULE_REGISTER(lora_tcp, CONFIG_LORA_TCP_LOG_LEVEL);

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

	lora_tcp_net_init(dev, cb);

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
	struct lora_tcp_device *dev = lora_tcp_device_get_by_id(dest_id);
	if (!dev) {
		LOG_ERR("Device not found");
		return -ENODEV;
	}

	struct lora_tcp_packet *pkt = &dev->send_packet;

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

struct lora_shell_data {
	uint8_t dev_id;
	uint8_t data[CONFIG_LORA_TCP_DATA_MAX_SIZE];
	uint8_t data_len;
};

K_MSGQ_DEFINE(lora_shell_msgq, sizeof(struct lora_shell_data), 4, 4);

static void shell_cmd_thread(void *, void *, void *)
{
	struct lora_shell_data dat;
	uint8_t rsp[CONFIG_LORA_TCP_DATA_MAX_SIZE];
	size_t rsp_len;

	while (k_msgq_get(&lora_shell_msgq, &dat, K_FOREVER) == 0) {
		LOG_HEXDUMP_WRN(dat.data, dat.data_len, "Sending:");
		lora_tcp_send(dat.dev_id, dat.data, dat.data_len, rsp, &rsp_len);
		LOG_HEXDUMP_WRN(rsp, rsp_len, "Response:");
	}

	LOG_ERR("shell cmd thread finished");
}

K_THREAD_DEFINE(lora_shell_tid, 1024, shell_cmd_thread, NULL, NULL, NULL, 5, 0, 0);

/**
 * @brief Shell command to test lora_tcp functionality
 */
static int control_ping(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);

	char *end;

	struct lora_shell_data dat;

	dat.dev_id = strtol(argv[1], &end, 10);

	if (end == argv[1] || *end != '\0') {
		LOG_ERR("Invalid dest");
		return -EINVAL;
	}

	dat.data_len = strlen(argv[2]);
	memcpy(dat.data, argv[2], dat.data_len);

	k_msgq_put(&lora_shell_msgq, &dat, K_FOREVER);

	LOG_INF("Sending data: %s", dat.data);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lora_tcp_sub,
			       SHELL_CMD_ARG(msg, NULL, "ping the other device", control_ping, 3,
					     0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tcp, &lora_tcp_sub, "Demo commands", NULL);

#endif /* ifdef CONFIG_LORA_TCP_SHELL */
