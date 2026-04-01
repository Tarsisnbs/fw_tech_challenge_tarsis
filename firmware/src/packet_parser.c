#include <stddef.h>
#include <stdint.h>
#include "packet_parser.h"


static uint8_t crc8_update(uint8_t crc, uint8_t data){
    crc ^= data; 
    for (uint8_t i = 0; i < 8; i++){
        if (crc & 0x80){
            crc = (crc << 1) ^ 0x31;
        }
        else{
            crc <<= 1;
        }
    }

    return crc;
}

void parser_init(parser_t *ptr_parser) {
    ptr_parser->state = STATE_WAIT_HEADER1;
    ptr_parser->crc = 0;
    ptr_parser->payload_index = 0;
}

bool parse_byte(parser_t *ptr_parser, uint8_t byte, raw_packet_t *ptr_pkt){
    switch (ptr_parser->state)
    {
    case STATE_WAIT_HEADER1:
        snprintf("STATE_WAIT_HEADER1\n");
        if(byte == PKT_HEADER_1){
            ptr_parser->state = STATE_WAIT_HEADER2;
        } 
        break;
    
    case STATE_WAIT_HEADER2: 
        snprintf("STATE_WAIT_HEADER2\n");
        if(byte == PKT_HEADER_2){
            ptr_parser->state = STATE_GET_SENSOR_ID;
        }
        else if (byte == PKT_HEADER_1) {
            //smart resinc to evite noise like |0xA5|0x5A|0xA5|...|
            ptr_parser->state = STATE_WAIT_HEADER2;

        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_SENSOR_ID:
        snprintf("STATE_GET_SENSOR_ID\n");
        if(byte <= 0x03){
            ptr_pkt->sensor_id = byte; 
            ptr_parser->state = STATE_GET_PAYLOAD_LEN;
        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_PAYLOAD_LEN:
        snprintf("STATE_GET_PAYLOAD_LEN\n");
        if(byte >= 1 && byte <= MAX_PAYLOAD_SIZE){
           ptr_pkt->payload_len = byte;     
           ptr_parser->payload_index = 0; 
           ptr_parser->state = STATE_GET_PAYLOAD;

        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_PAYLOAD:
        snprintf("STATE_GET_PAYLOAD\n");
        ptr_pkt->payload[ptr_parser->payload_index++] = byte;
        
        if(ptr_parser->payload_index >= ptr_pkt->payload_len){
            ptr_parser->state = STATE_GET_CRC;
        }
        break;
    
        //will be implement CRC calc.
    case STATE_GET_CRC:
        //next pkt 
        snprintf("STATE_GET_CRC\n");
        ptr_parser->state = STATE_WAIT_HEADER1;
        ptr_parser->payload_index = 0;

        if(ptr_pkt->crc == ptr_parser->crc){
            return true;
        }
    
    default: 
        snprintf("default \n");
        parser_init(ptr_parser);
        break;
    }

    return false;
}