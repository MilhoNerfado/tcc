//
// Created by milho on 8/12/24.
//

#ifndef LORA_TCP_PACKET_H
#define LORA_TCP_PACKET_H

#include <zephyr/kernel.h>

#define LORA_TCP_PACKET_MAX_SIZE                                                                   \
	(CONFIG_LORA_TCP_DATA_MAX_SIZE + sizeof(struct lora_tcp_packet_header))

enum lora_tcp_packet_status {
	LORA_TCP_PACKET_STATUS_OK = 0,
	LORA_TCP_PACKET_STATUS_REFUSED = 1,
	LORA_TCP_PACKET_STATUS_TIMEOUT = 2,
	LORA_TCP_PACKET_STATUS_BUSY = 3,
} __attribute__((packed));

struct lora_tcp_data {
	uint8_t buffer[CONFIG_LORA_TCP_DATA_MAX_SIZE];
	size_t len;
} __attribute__((packed));

struct lora_tcp_packet_header {
	uint8_t destination_id;
	uint8_t sender_id;
	uint8_t is_sync: 1;
	uint8_t is_ack: 1;
	enum lora_tcp_packet_status status: 2;
	uint8_t reserved: 4;
	uint32_t crc;
} __attribute__((packed));

struct lora_tcp_packet {
	struct lora_tcp_packet_header header;
	struct lora_tcp_data data;
} __attribute__((packed));

int lora_tcp_packet_build(struct lora_tcp_packet *packet, uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE],
			  size_t *buffer_length);

int lora_tcp_packet_unpack(const uint8_t *data, const size_t data_len,
			   struct lora_tcp_packet *packet);

#endif // LORA_TCP_PACKET_H
