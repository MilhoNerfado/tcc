//
// Created by milho on 8/12/24.
//

#ifndef LORA_TCP_PACKET_H
#define LORA_TCP_PACKET_H

#include <zephyr/kernel.h>

#define LORA_TCP_PACKET_MAX_SIZE                                                                   \
	(CONFIG_LORA_TCP_DATA_MAX_SIZE + sizeof(struct lora_tcp_packet_header))

struct lora_tcp_packet_header {
	uint8_t destination_id;
	uint8_t sender_id;
	bool is_ack;
	uint8_t pkt_id;
	uint32_t crc;
};

struct lora_tcp_packet {
	struct lora_tcp_packet_header header;
	uint8_t data[CONFIG_LORA_TCP_DATA_MAX_SIZE];
	size_t data_len;
};

int lora_tcp_packet_build(struct lora_tcp_packet *packet, uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE],
			  size_t *buffer_length);

int lora_tcp_packet_unpack(const uint8_t *data, const size_t data_len,
			   struct lora_tcp_packet *packet);

#endif // LORA_TCP_PACKET_H
