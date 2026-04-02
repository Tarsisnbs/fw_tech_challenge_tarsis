/**
 * @file fw_main.c
 * @brief Firmware implementation - CANDIDATE IMPLEMENTS HERE
 *
 * This file contains stub implementations for the firmware entry points.
 * You must implement the required functionality according to the challenge
 * requirements documented in README.md.
 *
 * IMPORTANT:
 * - Use ONLY static allocation for all FreeRTOS objects
 * - Use xTaskCreateStatic(), xQueueCreateStatic(), xSemaphoreCreateMutexStatic()
 * - Do NOT use any dynamic allocation (pvPortMalloc, malloc, etc.)
 * - You may create additional .c/.h files in firmware/src/ as needed
 * - Do NOT modify any files marked "DO NOT MODIFY"
 */

#include "fw_main.h"
#include "hal_uart.h"
#include "hal_gpio.h"

//#include "packet_parser.h"
#include <stddef.h>
#include <stdint.h>
//#include "packet_parser.h"
#include "hal_uart.h"



#define PKT_HEADER_1 0xA5
#define PKT_HEADER_2 0x5A
#define MAX_PAYLOAD_SIZE 16 

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

typedef struct{
    uint8_t sensor_id;
    uint8_t payload_len;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t crc;
}raw_packet_t;

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
        hal_uart1_send_str("STATE_WAIT_HEADER1\n");
        if(byte == PKT_HEADER_1){
            ptr_parser->state = STATE_WAIT_HEADER2;
        } 
        break;
    
    case STATE_WAIT_HEADER2: 
        hal_uart1_send_str("STATE_WAIT_HEADER2\n");
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
        hal_uart1_send_str("STATE_GET_SENSOR_ID\n");
        if(byte <= 0x03){
            ptr_pkt->sensor_id = byte; 
            ptr_parser->state = STATE_GET_PAYLOAD_LEN;
        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_PAYLOAD_LEN:
        hal_uart1_send_str("STATE_GET_PAYLOAD_LEN\n");
        if(byte >= 1 && byte <= MAX_PAYLOAD_SIZE){
           ptr_pkt->payload_len = byte;     
           ptr_parser->payload_index = 0; 
           ptr_parser->state = STATE_GET_PAYLOAD;

        }
        else ptr_parser->state = STATE_WAIT_HEADER1;
        break;
    
    case STATE_GET_PAYLOAD:
        hal_uart1_send_str("STATE_GET_PAYLOAD\n");
        ptr_pkt->payload[ptr_parser->payload_index++] = byte;
        
        if(ptr_parser->payload_index >= ptr_pkt->payload_len){
            ptr_parser->state = STATE_GET_CRC;
        }
        break;
    
        //will be implement CRC calc.
    case STATE_GET_CRC:
        //next pkt 
        hal_uart1_send_str("STATE_GET_CRC\n");
        ptr_parser->state = STATE_WAIT_HEADER1;
        ptr_parser->payload_index = 0;

        if(ptr_pkt->crc == ptr_parser->crc){
            return true;
        }
    
    default: 
        hal_uart1_send_str("default \n");
        parser_init(ptr_parser);
        break;
    }

    return false;
}


static parser_t parser; 
static raw_packet_t pkt;

//function to send hex byte
static void send_hex_byte(uint8_t byte) {
    // Tabela de conversão (look-up table)
    const char hex_digits[] = "0123456789ABCDEF";
    
    char buffer[3]; // 2 dígitos + null terminator
    buffer[0] = hex_digits[(byte >> 4) & 0x0F]; // Pega os 4 bits superiores
    buffer[1] = hex_digits[byte & 0x0F];        // Pega os 4 bits inferiores
    buffer[2] = '\0';

    hal_uart1_send_str(buffer);
    hal_uart1_send_str(" "); // Espaço entre os bytes
}

// Callback UART2 (byte a byte)
static void uart2_rx_callback(uint8_t byte) {
    if (parse_byte(&parser, byte, &pkt)) {
        // Pacote válido detectado
        hal_uart1_send_str("\r\n[OK] Packet received\r\n");
        hal_uart1_send_str("Payload: ");

        for (uint8_t i = 0; i < pkt.payload_len; i++) {
            send_hex_byte(pkt.payload[i]);
        }

        hal_uart1_send_str("wait next packet\r\n> ");
    }
}

void fw_init(void) {
    // UART debug
    hal_uart1_init(115200);

    hal_uart1_send_str("\r\n==============================\r\n");
    hal_uart1_send_str("FSM Parser Test (NO RTOS)\r\n");
    hal_uart1_send_str("==============================\r\n");
    hal_uart1_send_str("Send raw bytes via UART2\r\n");
    hal_uart1_send_str("> ");

    // Inicializa parser
    parser_init(&parser);

    // UART de entrada (parser)
    hal_uart2_init(115200, uart2_rx_callback);

    // LEDs (opcional)
    hal_gpio_pa5_init();

}
void fw_run(void) {
    /* TODO: Start FreeRTOS scheduler */
    uint8_t byte;

    while (1) {
        for (volatile int i = 0; i < 100000; i++);
    }
}
