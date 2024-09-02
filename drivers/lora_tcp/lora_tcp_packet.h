//
// Created by milho on 8/12/24.
//

#ifndef LORA_TCP_PACKET_H
#define LORA_TCP_PACKET_H

#include <zephyr/kernel.h>

/* TODO: Move to prj.conf */
#define CONFIG_LORA_TCP_DATA_MAX_SIZE 10

#define LORA_TCP_PACKET_MAX_SIZE (CONFIG_LORA_TCP_DATA_MAX_SIZE + sizeof(struct lora_tcp_packet_header))

struct lora_tcp_data
{
    uint8_t buffer[CONFIG_LORA_TCP_DATA_MAX_SIZE];
    size_t len;
};

struct lora_tcp_packet_header
{
    uint8_t destination_id;
    uint8_t sender_id;
    uint32_t crc;
} __attribute__((packed));

struct lora_tcp_packet
{
    struct lora_tcp_packet_header header;
    struct lora_tcp_data data;
};


int lora_tcp_packet_build(const uint8_t dest_id, const uint8_t sender_id, const uint32_t crc,
                          const uint8_t* data, const size_t data_len,
                          uint8_t buffer[LORA_TCP_PACKET_MAX_SIZE], size_t* buffer_length);

int lora_tcp_packet_unpack(const uint8_t* data, const size_t data_len,
                           struct lora_tcp_packet* packet);

#endif // LORA_TCP_PACKET_H
