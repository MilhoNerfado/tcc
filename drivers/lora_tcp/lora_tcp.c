//
// Created by milho on 8/2/24.
//

#include <app/drivers/lora_tcp.h>

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <errno.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

enum lora_tcp_packet_header_flags {
	ACK = 0,
	SYNC,
	DATA,
};

struct lora_tcp_packet {
	uint8_t flags;
	uint8_t checksum;
	uint32_t data_len;
	uint8_t *data;
};


