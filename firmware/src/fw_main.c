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
#include "parser.h"

#include <stddef.h>
#include <stdint.h>

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

static parser_t parser; 
static raw_packet_t pkt;

// Callback UART2 (byte a byte)
static void uart2_rx_callback(uint8_t byte) {
    if (parse_byte(&parser, byte, &pkt)) {
        
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
