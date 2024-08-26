
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/shell/shell.h>

#include "app/drivers/lora_tcp.h"

LOG_MODULE_REGISTER(main);

static struct {
	bool metadata;
	bool rsp;
} relay = {
	.metadata = false,
	.rsp = false,
};

int main(void)
{
	lora_tcp_init(1);

	LOG_WRN(" --- SYSTEM INIT --- ");

	k_sleep(K_FOREVER);

	return 0;
}

/* --- */

static int control_toggle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = lora_tcp_send(0, "77720", 5);
	if (err != 0) {
		LOG_WRN("failed lora_send | %d\n", err);
		return -1;
	}

	return 0;
}

static int control_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = lora_tcp_send(0, "77700", 5);
	if (err != 0) {
		LOG_WRN("failed lora_send | %d\n", err);
		return -1;
	}

	return 0;
}

static int control_set(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		LOG_ERR("Incorrect argument amount | argc: %lu", argc);
		return -1;
	}

	if (strlen(argv[1]) != 1) {
		LOG_ERR("data too big | argv 0 size: %lu", strlen(argv[1]));
		return -1;
	}
	int err;
	char buffer[6] = {0};

	snprintf(buffer, 6, "7771%s", argv[1]);

	shell_print(sh, "buffer: %s", buffer);

	err = lora_tcp_send(0, "7771", 5);
	if (err != 0) {
		LOG_WRN("failed lora_send | %d\n", err);
		return -1;
	}

	return 0;
}

static int control_ping(const struct shell *sh, size_t argc, char **argv)
{
	char ping[] = "ping";
	lora_tcp_send(0, ping, strlen(ping));
	return 0;
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(controller,
			       SHELL_CMD(set, NULL, "Set Relay value to x", control_set),
			       SHELL_CMD(toggle, NULL, "Toggle Relay", control_toggle),
			       SHELL_CMD(get, NULL, "Get relay state", control_get),
			       SHELL_CMD(ping, NULL, "ping the other device", control_ping),
			       SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(control, &controller, "Demo commands", NULL);
