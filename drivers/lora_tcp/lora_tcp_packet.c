//
// Created by milho on 8/12/24.
//

#include "lora_tcp_packet.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>

LOG_MODULE_REGISTER(lora_tcp_packet);

int lora_tcp_packet_build(struct lora_tcp_packet *packet, uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE],
			  size_t *buffer_length)
{
	CHECKIF(packet == NULL || buffer == NULL || buffer_length == NULL) {
		LOG_ERR("Invalid parameters");
		return -1;
	}

	memcpy(buffer, packet, packet->data_len + sizeof(struct lora_tcp_packet_header));

	*buffer_length = packet->data_len + sizeof(struct lora_tcp_packet_header);

	return 0;
}

int lora_tcp_packet_unpack(const uint8_t *data, const size_t data_len,
			   struct lora_tcp_packet *packet)
{
	CHECKIF(data == NULL || packet == NULL) {
		LOG_ERR("Invalid pointer");
		return -1;
	};

	CHECKIF(data_len < sizeof(struct lora_tcp_packet_header) ||
		data_len > LORA_TCP_PACKET_MAX_SIZE) {
		LOG_ERR("Invalid data length");
		return -1;
	};

	memcpy(packet, data, data_len);

	packet->data_len = data_len - sizeof(struct lora_tcp_packet_header);

	return 0;
}

