#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"

LOG_MODULE_REGISTER(main);

void relay(uint8_t *data, size_t data_len, uint8_t *response, size_t *response_size)
{
	LOG_WRN("Relay ran | data lenght: %lu", data_len);
}

int main(void)
{
	lora_tcp_init(3, 0, relay);

	lora_tcp_register(1, 0);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- ACTUATOR --- ");

	return 0;
}
