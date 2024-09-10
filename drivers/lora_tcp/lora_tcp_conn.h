
#ifndef LORA_TCP_CONN_H
#define LORA_TCP_CONN_H

#include <zephyr/kernel.h>

struct lora_tcp_conn_connection {
	uint8_t device_id;
	bool is_connected;
};

int lora_tcp_conn_start(uint8_t id);

int lora_tcp_conn_end(void);

int lora_tcp_conn_find(uint8_t id, struct lora_tcp_conn_connection **conn);

#endif /* LORA_TCP_CONN_H */
