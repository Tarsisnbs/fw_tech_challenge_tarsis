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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/**
 * @brief Firmware initialization
 *
 * Called once from main() before the scheduler starts.
 *
 * TODO: Implement the following:
 * 1. Initialize HAL peripherals (UART1, UART2, GPIO PA5, PA6, PB0)
 * 2. Create all FreeRTOS tasks using xTaskCreateStatic()
 *    - Watchdog LED task (toggles PA5 every 500ms)
 *    - Sensor packet processing task
 *    - Fault monitor task (checks sensor timeouts every 200ms)
 *    - Diagnostic shell task (handles UART1 commands)
 * 3. Create FreeRTOS queues using xQueueCreateStatic()
 *    - Queue for UART2 RX bytes (ISR to task handoff)
 *    - Queue for UART1 RX bytes (if needed for command parsing)
 * 4. Create FreeRTOS mutexes using xSemaphoreCreateMutexStatic()
 *    - Mutex for sensor data store access
 *    - Mutex for UART1 TX access (if needed)
 * 5. Initialize sensor storage structures (statically allocated)
 *
 * All memory must be statically allocated - declare static arrays for:
 * - Task stacks (StackType_t)
 * - Task control blocks (StaticTask_t)
 * - Queue storage (uint8_t arrays)
 * - Queue structures (StaticQueue_t)
 * - Mutex structures (StaticSemaphore_t)
 */

#include <packet_parser.h>

// Callback UART2 (byte a byte)
static void uart2_rx_callback(uint8_t byte) {
    if (parser_process_byte(&parser, byte, &pkt)) {
        // Pacote válido detectado
        hal_uart1_send_str("\r\n[OK] Packet received\r\n");
        char msg[64];

        snprintf(msg, sizeof(msg),
                 "ID=%d LEN=%d CRC=0x%02X\r\n",
                 pkt.sensor_id,
                 pkt.payload_len,
                 pkt.crc);

        hal_uart1_send_str(msg);
        hal_uart1_send_str("Payload: ");

        for (uint8_t i = 0; i < pkt.payload_len; i++) {
            char b[8];
            snprintf(b, sizeof(b), "%02X ", pkt.payload[i]);
            hal_uart1_send_str(b);
        }

        hal_uart1_send_str("\r\n> ");
    }
}

static parser_t parser; 
static raw_packet_t pkt;

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


void fw_run(void) {
    /* TODO: Start FreeRTOS scheduler */
    uint8_t byte;

    while (1) {
        for (volatile int i = 0; i < 100000; i++);
    }
}
