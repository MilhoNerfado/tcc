
#ifndef H_APP_LORA_UDP
#define H_APP_LORA_UDP

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <sys/types.h>

#define NUM_OF_OUTPUTS 2

int init_radio(void);

struct chan_out {
	bool outputs[NUM_OF_OUTPUTS];
};

struct lora_udp_header {
	uint16_t sender_id;
	uint16_t destination_id;
	uint32_t crc;
	uint8_t data_len;
};

struct lora_udp_packet {
	struct lora_udp_header header;
	void *data;
};

struct pkg_data_actuator {
	struct chan_out want;
};

struct pkg_data_controler {
	struct chan_out state;
	struct chan_out def_state;
	time_t timeout;
};

#endif /* H_APP_LORA_UDP */
