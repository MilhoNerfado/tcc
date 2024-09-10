//
// Created by milho on 8/2/24.
//

#ifndef LORA_TCP_H
#define LORA_TCP_H

#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>

int lora_tcp_init(uint8_t dev_id, uint8_t dev_key_id, struct k_fifo *fifo);

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len);

#endif // LORA_TCP_H
