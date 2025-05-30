#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_WRN);

#include <zephyr/drivers/lora.h>
#include "libs/lora_udp.h"

int main(void)
{
	LOG_WRN(" --- ACTUATOR INIT --- ");

	init_radio();

	while (true) {
		k_msleep(1000);
	}
}
