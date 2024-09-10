
#ifndef LORA_TCP_CONN_H
#define LORA_TCP_CONN_H

#include <zephyr/kernel.h>
#include "lora_tcp_device.h"

int lora_tcp_conn_start(uint8_t id);

int lora_tcp_conn_end(void);

int lora_tcp_conn_get_connected(struct lora_tcp_device **dev);

int lora_tcp_conn_get_old(struct lora_tcp_device **dev);

#endif /* LORA_TCP_CONN_H */
