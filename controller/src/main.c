
#include <zephyr/logging/log.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/shell/shell.h>

LOG_MODULE_REGISTER(main);

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

const struct device *const lora_device = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

struct lora_modem_config lora_configuration = {
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

K_MUTEX_DEFINE(relay_mutex);

static struct {
	bool metadata;
	bool rsp;
} relay = {
	.metadata = false,
	.rsp = false,
};

static int lora_init(void);

int main(void)
{
	lora_init();

	LOG_WRN("SYSTEM INIT");

	return 0;
}

static int lora_init(void)
{
	int err;

	if (!device_is_ready(lora_device)) {
		LOG_ERR("%s Device not ready", lora_device->name);
		return -ENODEV;
	}

	err = lora_config(lora_device, &lora_configuration);
	if (err < 0) {
		LOG_ERR("LoRa config failed");
		return -EIO;
	}

	return 0;
}

/* --- */

static int control_toggle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = lora_send(lora_device, "77720", 5);
	if (err != 0) {
		LOG_WRN("failed lora_send | %d\n", err);
		return -1;
	}

	return 0;
}

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size, int16_t rssi,
		     int8_t snr)
{
	static int cnt;

	ARG_UNUSED(dev);

	LOG_INF("Received data: %s (RSSI:%ddBm, SNR:%ddBm)", data, rssi, snr);

	if (size != 6) {
		return;
	}

	if (memcmp(data, "666000", 6) != 0) {
		relay.metadata = true;
		relay.rsp = 0;
		LOG_INF("Found 0");
		lora_recv_async(dev, NULL);
		k_mutex_unlock(&relay_mutex);
		return;
	}

	if (memcmp(data, "666001", 6) != 0) {
		relay.metadata = true;
		relay.rsp = 1;
		LOG_INF("Found 1");
		lora_recv_async(dev, NULL);
		k_mutex_unlock(&relay_mutex);
		return;
	}

	/* Stop receiving after 10 packets */
	if (++cnt == 10) {
		LOG_INF("Stopping packet receptions");
		LOG_INF("Found none");
		relay.metadata = false;
		lora_recv_async(dev, NULL);
		k_mutex_unlock(&relay_mutex);
		return;
	}
}

static int control_get(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = lora_send(lora_device, "77700", 5);
	if (err != 0) {
		LOG_WRN("failed lora_send | %d\n", err);
		return -1;
	}

	lora_recv_async(lora_device, lora_receive_cb);

	k_mutex_lock(&relay_mutex, K_FOREVER);

	if (!relay.metadata) {
		return -1;
	}

	return 0;
}

static int control_set(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 1) {
		return -1;
	}

	if (strlen(argv[0]) != 1) {
		return -1;
	}
	int err;
	char buffer[6] = {0};

	snprintf(buffer, 6, "7771%s", argv[0]);
	err = lora_send(lora_device, "7771", 5);
	if (err != 0) {
		LOG_WRN("failed lora_send | %d\n", err);
		return -1;
	}

	return 0;
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(controller,
			       SHELL_CMD(set, NULL, "Set Relay value to x", control_set),
			       SHELL_CMD(toggle, NULL, "Toggle Relay", control_toggle),
			       SHELL_CMD(get, NULL, "Get relay state", control_get),
			       SHELL_SUBCMD_SET_END);
/* Creating root (level 0) command "demo" without a handler */
SHELL_CMD_REGISTER(control, &controller, "Demo commands", NULL);
