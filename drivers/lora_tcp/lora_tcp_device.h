//
// Created by milho on 8/12/24.
//

#ifndef LORA_TCP_DEVICE_H
#define LORA_TCP_DEVICE_H

#include <zephyr/kernel.h>

struct lora_tcp_device_ids {
	uint8_t dev_id;
	uint8_t pub_key_id;
	uint8_t priv_key_id;
};

struct lora_tcp_device_master {
	uint8_t master_ack;
	uint8_t slave_ack;
	uint8_t last_pkg[255];
	struct k_timer timer;
	// uint8_t tries; // TODO check if it's needed (the timer might already have this feature)
};

struct lora_tcp_device_slave {
	uint8_t master_ack;
	uint8_t slave_ack;
	uint8_t last_pkg[255];
	// TODO Add callback pointer
};

struct lora_tcp_device {
	struct lora_tcp_device_ids ids;
	struct lora_tcp_device_master master;
	struct lora_tcp_device_slave slave;
};

#endif // LORA_TCP_DEVICE_H
