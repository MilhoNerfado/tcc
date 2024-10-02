//
// Created by milho on 8/12/24.
//

#ifndef LORA_TCP_DEVICE_H
#define LORA_TCP_DEVICE_H

#include <zephyr/kernel.h>

struct lora_tcp_device {
	bool is_registered;
	uint8_t id;
	uint8_t snd_pkt_id;
};

void lora_tcp_device_self_set(uint8_t id, uint8_t key_id);

struct lora_tcp_device *lora_tcp_device_self_get(void);

int lora_tcp_device_register(uint8_t id, uint8_t key_id);

int lora_tcp_device_unregister(uint8_t id);

struct lora_tcp_device *lora_tcp_device_get_by_id(uint8_t id);

int lora_tcp_device_get_pkt_id(struct lora_tcp_device *device);

int lora_tcp_device_update_pkt_id(struct lora_tcp_device *device);

#endif // LORA_TCP_DEVICE_H
