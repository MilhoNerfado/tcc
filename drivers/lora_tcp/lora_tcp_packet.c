//
// Created by milho on 8/12/24.
//

#include "lora_tcp_packet.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/check.h>


LOG_MODULE_REGISTER(lora_tcp_packet);


int lora_tcp_packet_build(const uint8_t dest_id, const uint8_t sender_id, const uint32_t crc,
                          const uint8_t *data, const size_t data_len,
                          uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE], size_t *buffer_length)
{
	CHECKIF(data == NULL || buffer == NULL || buffer_length == NULL) {
		LOG_ERR("Invalid parameters");
		return -1;
	}

	const struct lora_tcp_packet_header header = {
		.destination_id = dest_id,
		.sender_id = sender_id,
		.crc = crc,
	};

	memcpy(buffer, &header, sizeof(header));

	memcpy(buffer + sizeof(header), data, data_len);

	*buffer_length = data_len + sizeof(header);

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

	memcpy(&packet->header, data, sizeof(struct lora_tcp_packet_header));

	memcpy(packet->data.buffer, data + sizeof(struct lora_tcp_packet_header),
	       data_len - sizeof(struct lora_tcp_packet_header));

	packet->data.len = data_len - sizeof(struct lora_tcp_packet_header);

	return 0;
}