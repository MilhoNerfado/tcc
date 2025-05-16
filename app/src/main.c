#include "zephyr/drivers/gpio.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

/* #define DEFAULT_RADIO_NODE DT_ALIAS(lora0) */

LOG_MODULE_REGISTER(main);

#include <zephyr/drivers/lora.h>

int main(void)
{
	LOG_WRN(" --- SYSTEM INIT --- ");

	while (true) {
		k_msleep(1000);
	}
}
