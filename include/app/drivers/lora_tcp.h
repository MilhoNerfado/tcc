//
// Created by milho on 8/2/24.
//

#ifndef LORA_TCP_H
#define LORA_TCP_H

#include <zephyr/kernel.h>

#define LORA_TCP_DATA_MAX_SIZE 30

struct lora_tcp_packet_header {
	uint8_t destination_id;
	uint8_t sender_id;
	uint16_t crc;
} __attribute__((packed));

struct lora_tcp_packet_encrypted {
	uint16_t destination_ack;
	uint16_t sender_ack;
	uint8_t flags: 4;
	uint8_t data_len: 4;
	uint8_t data[LORA_TCP_DATA_MAX_SIZE];
} __attribute__((packed));

struct lora_tcp_packet {
	struct lora_tcp_packet_header header;
	struct lora_tcp_packet_encrypted encrypted;
} __attribute__((packed));

enum lora_tcp_flags {
	LORA_TCP_FLAG_REQUEST = BIT(0),
	LORA_TCP_FLAG_ACK = BIT(1),
	LORA_TCP_FLAG_SYNC = BIT(2),
};

int lora_tcp_init(uint8_t dev_id);

int lora_tcp_send(const uint8_t dest_id, uint8_t *data, const uint8_t data_len);

#endif // LORA_TCP_H
