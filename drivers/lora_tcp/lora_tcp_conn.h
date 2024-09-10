
#ifndef LORA_TCP_CONN_H
#define LORA_TCP_CONN_H

#include <zephyr/kernel.h>
#include "lora_tcp_device.h"

int lora_tcp_conn_start(struct lora_tcp_device *device);

int lora_tcp_conn_end(void);

struct lora_tcp_device *lora_tcp_conn_get_connected(void);

struct lora_tcp_device *lora_tcp_conn_get_old(void);

#endif /* LORA_TCP_CONN_H */
