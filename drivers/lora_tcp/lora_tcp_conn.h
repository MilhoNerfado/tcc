
#ifndef LORA_TCP_CONN_H
#define LORA_TCP_CONN_H

#include <zephyr/kernel.h>
#include "lora_tcp_device.h"
#include "lora_tcp_packet.h"

void lora_tcp_conn_resolve(struct lora_tcp_packet *recv_packet);

int lora_tcp_conn_start(struct lora_tcp_device *device, uint8_t pkt_id);

int lora_tcp_conn_end(void);

struct lora_tcp_device *lora_tcp_conn_get_connected(void);

int lora_tcp_conn_get_connected_pkt_id(void);

struct lora_tcp_device *lora_tcp_conn_get_old(void);

int lora_tcp_conn_get_old_pkt_id(void);

#endif /* LORA_TCP_CONN_H */
