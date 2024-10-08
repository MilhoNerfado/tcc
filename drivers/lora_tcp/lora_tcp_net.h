//
// Created by milho on 10/2/24.
//

#ifndef LORA_TCP_NET_H
#define LORA_TCP_NET_H

#include <zephyr/device.h>

#include "lora_tcp_packet.h"

int lora_tcp_net_init(const struct device * dev);

int lora_tcp_net_send(struct lora_tcp_packet *pkt);

#endif //LORA_TCP_NET_H
