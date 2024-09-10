#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"

LOG_MODULE_REGISTER(main);

K_FIFO_DEFINE(lora_fifo);

int main(void)
{
	lora_tcp_init(1, 0, &lora_fifo);

	LOG_WRN(" --- SYSTEM INIT --- ");
	LOG_WRN(" --- CONTROLLER --- ");

	return 0;
}
