#include "parser.h"
#include "crc8.h"

#define PKT_HEADER_1 0xA5
#define PKT_HEADER_2 0x5A

typedef enum {
    STATE_WAIT_HEADER1,
    STATE_WAIT_HEADER2,
    STATE_GET_SENSOR_ID,
    STATE_GET_PAYLOAD_LEN,
    STATE_GET_PAYLOAD,
    STATE_GET_CRC
}parser_state_t;

void parser_init(parser_t *ptr_parser) {
    ptr_parser->state = STATE_WAIT_HEADER1;
    ptr_parser->crc = 0;
    ptr_parser->payload_index = 0;
}

bool parse_byte(parser_t *ptr_parser, uint8_t byte, raw_packet_t *ptr_pkt){
    switch (ptr_parser->state)
    {
    case STATE_WAIT_HEADER1:
        if(byte == PKT_HEADER_1){
            ptr_parser->crc = 0;
            ptr_parser->payload_index = 0;
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            ptr_parser->state = STATE_WAIT_HEADER2;
        } 
        break;
    
    case STATE_WAIT_HEADER2: 
        if(byte == PKT_HEADER_2){
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            ptr_parser->state = STATE_GET_SENSOR_ID;
        }
        else if (byte == PKT_HEADER_1) {
            //smart resinc to evite noise like |0xA5|0x5A|0xA5|...|
            ptr_parser->crc = 0;
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            ptr_parser->state = STATE_WAIT_HEADER2;

        }
        else {
            ptr_parser->state = STATE_WAIT_HEADER1;
        }
        break;
    
    case STATE_GET_SENSOR_ID:
        if(byte <= 0x03){
            ptr_pkt->sensor_id = byte; 
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            ptr_parser->state = STATE_GET_PAYLOAD_LEN;
        }
        else{
            ptr_parser->state = STATE_WAIT_HEADER1;
        } 
        break;
    
    case STATE_GET_PAYLOAD_LEN:
        if(byte >= 1 && byte <= MAX_PAYLOAD_SIZE){
           ptr_pkt->payload_len = byte;   
           ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
           ptr_parser->payload_index = 0; 
           ptr_parser->state = STATE_GET_PAYLOAD;

        }
        else{
           ptr_parser->state = STATE_WAIT_HEADER1;
        } 
        break;
    
    case STATE_GET_PAYLOAD:
        ptr_pkt->payload[ptr_parser->payload_index++] = byte;
        ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
        
        if(ptr_parser->payload_index >= ptr_pkt->payload_len){
            ptr_parser->state = STATE_GET_CRC;
        }
        break;

    case STATE_GET_CRC:
        ptr_pkt->crc = byte;
        ptr_parser->state = STATE_WAIT_HEADER1;
        ptr_parser->payload_index = 0;
        
        if(ptr_pkt->crc == ptr_parser->crc){
            return true;
        }
        break;
    default: 
        parser_init(ptr_parser);
        break;
    }
    return false;
}