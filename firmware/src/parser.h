#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_PAYLOAD_SIZE 16 

typedef struct{
    uint8_t sensor_id;
    uint8_t payload_len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t crc;
}raw_packet_t;

typedef struct{
    uint8_t state;
    uint8_t crc;
    uint8_t payload_index;
}parser_t;

void parser_init(parser_t *ptr_parser);
bool parser_process_byte(parser_t *ptr_parser, uint8_t byte, raw_packet_t *ptr_pkt);

#endif