
#ifndef LORA_TCP_CONN_H
#define LORA_TCP_CONN_H

#include <zephyr/kernel.h>
#include "lora_tcp_device.h"
#include "lora_tcp_packet.h"

enum lora_tcp_conn_result {
	LORA_TCP_CONN_RESULT_OK = 0,
	LORA_TCP_CONN_RESULT_SEND_OLD,
	LORA_TCP_CONN_RESULT_SEND_CONNECTED,
	LORA_TCP_CONN_RESULT_SEND_REFUSED,
	_LORA_TCP_CONN_RESULT_MAXX,
};

void lora_tcp_conn_resolve(struct lora_tcp_packet *recv_packet,
			   enum lora_tcp_conn_result *resolve_result);

int lora_tcp_conn_start(struct lora_tcp_device *device, uint8_t pkt_id);

int lora_tcp_conn_end(void);

struct lora_tcp_device *lora_tcp_conn_get_connected(void);

int lora_tcp_conn_get_connected_pkt_id(void);

int lora_tcp_conn_save_packet(struct lora_tcp_packet *packet);

int lora_tcp_conn_copy_connected_packet(struct lora_tcp_packet *packet);

struct lora_tcp_device *lora_tcp_conn_get_old(void);

int lora_tcp_conn_get_old_pkt_id(void);

int lora_tcp_conn_copy_old_packet(struct lora_tcp_packet *packet);

#endif /* LORA_TCP_CONN_H */
