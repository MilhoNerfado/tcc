#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"


#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

LOG_MODULE_REGISTER(main);


void relay(uint8_t *data, size_t data_len, uint8_t *response, size_t *response_size)
{
	LOG_WRN("Relay ran | data lenght: %lu", data_len);
}

int main(void)
{
	lora_tcp_init(DEVICE_DT_GET(DEFAULT_RADIO_NODE),3, relay);

	lora_tcp_register(1);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- ACTUATOR --- ");

	return 0;
}
