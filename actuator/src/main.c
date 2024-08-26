/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#include "app/drivers/lora_tcp.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lora_receive);

int main(void)
{

	lora_tcp_init(0);
	k_sleep(K_FOREVER);
	return 0;
}
