//
// Created by milho on 8/2/24.
//

#include <app/drivers/lora_tcp.h>
#include "lora_tcp_core.h"

#include <zephyr/device.h>
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
	lora_tcp_core_cb cb;
} self = {
	.is_init = false,
	.cb = NULL,
};
/**
 * @brief Driver initialization function
 *
 * @param dev_id ID of the device to be used on P2P communication
 * @param fifo pointer to a FIFO to store received messages
 * @return 0 for OK, -X otherwise
 */
int lora_tcp_init(uint8_t dev_id, uint8_t dev_key_id, void *cb)
{
	if (self.is_init) {
		return 0;
	}

	CHECKIF(cb == NULL) {
		LOG_ERR("Invalid callback pointer");
		return -EINVAL;
	}

	lora_tcp_core_init(dev_id, dev_key_id, cb);

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
	return lora_tcp_core_send(dest_id, data, data_len, rsp, rsp_len);
}

int lora_tcp_register(const uint8_t id, const uint8_t key)
{
	return lora_tcp_device_register(id, key);
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
	char *end;
	size_t id;

	// id = strtol(argv[1], &end, 10);
	// if (end == argv[1] && *end != '\0') {
	// 	return -1;
	// }

	lora_tcp_send(1, ping, strlen(ping), NULL, NULL);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lora_tcp_sub,
			       SHELL_CMD(ping, NULL, "ping the other device", control_ping),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(lora_tcp, &lora_tcp_sub, "Demo commands", NULL);

#endif /* ifdef CONFIG_LORA_TCP_SHELL */
