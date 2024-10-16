//
// Created by milho on 10/2/24.
//

#ifndef LORA_TCP_NET_H
#define LORA_TCP_NET_H

#include <zephyr/device.h>

#include "lora_tcp_packet.h"

typedef void (*lora_tcp_cb)(uint8_t sender_id, uint8_t *data, size_t lenght, uint8_t *response,
			    size_t *response_len);

int lora_tcp_net_init(const struct device *dev, void *callback);

int lora_tcp_net_send(struct lora_tcp_packet *pkt, uint8_t *rsp, size_t *rsp_len);

#endif // LORA_TCP_NET_H
