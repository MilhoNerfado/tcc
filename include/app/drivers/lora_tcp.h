//
// Created by milho on 8/2/24.
//

#ifndef LORA_TCP_H
#define LORA_TCP_H

#include <zephyr/kernel.h>

int lora_tcp_init(uint8_t dev_id);

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len);

#endif // LORA_TCP_H
