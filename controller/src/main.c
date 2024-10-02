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
	lora_tcp_init(1, 0, relay);

	lora_tcp_register(3, 0);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- CONTROLLER --- ");

	k_sleep(K_SECONDS(5));

	lora_tcp_send(3, "ping", strlen("ping") + 1, NULL, NULL);

	return 0;
}
