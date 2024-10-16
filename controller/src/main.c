#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"


#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

LOG_MODULE_REGISTER(main);


#define SELF_ID 1

void relay(uint8_t id, uint8_t *data, size_t data_len, uint8_t *response, size_t *response_size)
{
	LOG_WRN("Relay ran | data lenght: %lu", data_len);
}

int main(void)
{

	uint8_t buffer[100];
	size_t buffer_len;

	lora_tcp_init(DEVICE_DT_GET(DEFAULT_RADIO_NODE),SELF_ID, relay);

	lora_tcp_register(3);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- CONTROLLER | ID: %d --- ", SELF_ID);

	// k_sleep(K_SECONDS(1));
	//
	// lora_tcp_send(3, "ping", strlen("ping"), buffer, &buffer_len);
	//
	// LOG_HEXDUMP_INF(buffer, buffer_len, "ping");

	return 0;
}
