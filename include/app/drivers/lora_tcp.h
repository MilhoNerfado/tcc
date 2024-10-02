//
// Created by milho on 8/2/24.
//

#ifndef LORA_TCP_H
#define LORA_TCP_H

#include <zephyr/kernel.h>

int lora_tcp_init(uint8_t dev_id, uint8_t dev_key_id, void *cb);

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len, uint8_t *rsp,
		  size_t *rsp_len);

int lora_tcp_register(const uint8_t id, const uint8_t key);

int lora_tcp_unregister(const uint8_t id);

#endif // LORA_TCP_H
