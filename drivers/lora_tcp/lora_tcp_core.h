
#ifndef LORA_TCP_CORE_H
#define LORA_TCP_CORE_H

#include <zephyr/kernel.h>
#include "lora_tcp_device.h"

typedef void (*lora_tcp_core_cb)(uint8_t *data, size_t data_len, uint8_t *response,
				 size_t *response_size);

int lora_tcp_core_init(uint8_t dev_id, uint8_t dev_key_id, lora_tcp_core_cb callback);

int lora_tcp_core_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len, uint8_t *rsp,
		       size_t *rsp_len);

#endif /* LORA_TCP_CORE_H */
