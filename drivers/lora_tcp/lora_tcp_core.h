
#ifndef LORA_TCP_CORE_H
#define LORA_TCP_CORE_H

#include <zephyr/kernel.h>

int lora_tcp_core_init(uint8_t dev_id, uint8_t dev_key_id);

int lora_tcp_core_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len);

#endif /* LORA_TCP_CORE_H */
