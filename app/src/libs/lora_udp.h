
#ifndef H_APP_LORA_UDP
#define H_APP_LORA_UDP

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <sys/types.h>

#define NUM_OF_OUTPUTS 2

struct lora_udp_header {
	uint16_t sender_id;
	uint16_t destination_id;
	uint32_t crc;
	uint16_t pkt_id;
	size_t data_len;
};

struct lora_udp_packet {
	struct lora_udp_header header;
	void *data;
};

struct chan_out {
	bool outputs[NUM_OF_OUTPUTS];
};

#endif /* H_APP_LORA_UDP */
