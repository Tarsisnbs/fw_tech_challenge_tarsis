#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef PACKET_PARSER_H
#define PACKET_PARSER_H


#define PKT_HEADER_1 0xA5
#define PKT_HEADER_2 0x5A
#define MAX_PAYLOAD_SIZE 16 

typdef struct {
    uint8_t sensor_id;
    uint8_t payload_len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t crc;
}raw_packet_t;

typedef enum {
    STATE_WAIT_HEADER1,
    STATE_WAIT_HEADER2,
    STATE_GET_SENSOR_ID,
    STATE_GET_PAYLOAD_LEN,
    STATE_GET_PAYLOAD,
    STATE_GET_CRC
}parser_state_t;

typedef struct{
    parser_state_t state;
    uint8_t crc;
    uint8_t payload_index;
}parser_t;

//init parser
void parser_init(parser_t *parser);

bool parse_process_byte(uint8_t byte, raw_packet_t *out_pkt);


