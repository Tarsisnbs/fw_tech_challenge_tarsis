#include "parser.h"

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
        hal_uart1_send_str("STATE_WAIT_HEADER1\n");
        if(byte == PKT_HEADER_1){
            ptr_parser->crc = 0;
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            send_hex_byte(byte);
            hal_uart1_send_str(" crc calculado = \n");
            send_hex_byte(ptr_parser->crc);
            ptr_parser->state = STATE_WAIT_HEADER2;
        } 
        break;
    
    case STATE_WAIT_HEADER2: 
        hal_uart1_send_str("STATE_WAIT_HEADER2\n");
        if(byte == PKT_HEADER_2){
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            send_hex_byte(byte);
            hal_uart1_send_str(" crc calculado = \n");
            send_hex_byte(ptr_parser->crc);
            ptr_parser->state = STATE_GET_SENSOR_ID;
        }
        else if (byte == PKT_HEADER_1) {
            //smart resinc to evite noise like |0xA5|0x5A|0xA5|...|
            //refactor
            ptr_parser->crc = 0;
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            send_hex_byte(byte);
            hal_uart1_send_str(" crc calculado = \n");
            send_hex_byte(ptr_parser->crc);
            ptr_parser->state = STATE_WAIT_HEADER2;

        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_SENSOR_ID:
        hal_uart1_send_str("STATE_GET_SENSOR_ID\n");
        if(byte <= 0x03){
            ptr_pkt->sensor_id = byte; 
            ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
            send_hex_byte(byte);
            hal_uart1_send_str(" crc calculado = \n");
            send_hex_byte(ptr_parser->crc);
            ptr_parser->state = STATE_GET_PAYLOAD_LEN;
        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_PAYLOAD_LEN:
        hal_uart1_send_str("STATE_GET_PAYLOAD_LEN\n");
        if(byte >= 1 && byte <= MAX_PAYLOAD_SIZE){
           ptr_pkt->payload_len = byte;   
           ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
           send_hex_byte(byte);
           hal_uart1_send_str(" crc calculado = \n");
           send_hex_byte(ptr_parser->crc);
           ptr_parser->payload_index = 0; 
           ptr_parser->state = STATE_GET_PAYLOAD;

        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_PAYLOAD:
        hal_uart1_send_str("STATE_GET_PAYLOAD\n");
        ptr_pkt->payload[ptr_parser->payload_index++] = byte;
        ptr_parser->crc = crc8_update(ptr_parser->crc, byte);
        send_hex_byte(byte);
        hal_uart1_send_str(" crc calculado = \n");
        send_hex_byte(ptr_parser->crc);
        
        if(ptr_parser->payload_index >= ptr_pkt->payload_len){
            hal_uart1_send_str("end of payload\n");
            
            ptr_parser->state = STATE_GET_CRC;
        }
        break;
    
        //will be implement CRC calc.
    case STATE_GET_CRC:
        //next pkt 
        ptr_pkt->crc = byte;
        hal_uart1_send_str("STATE_GET_CRC\n");
        ptr_parser->state = STATE_WAIT_HEADER1;
        ptr_parser->payload_index = 0;
        
        if(ptr_pkt->crc == ptr_parser->crc){
            hal_uart1_send_str("crc ok\n");
            return true;
        }
        else hal_uart1_send_str("crc error\n");
    default: 
        hal_uart1_send_str("default \n");
        parser_init(ptr_parser);
        break;
    }

    return false;
}