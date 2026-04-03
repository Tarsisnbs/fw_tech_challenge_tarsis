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

#include <stddef.h>
#include <stdint.h>

#define UART_QUEUE_LENGTH 64

static uint8_t uart_queue_storage[UART_QUEUE_LENGTH];
static StaticQueue_t uart_queue_struct;

static QueueHandle_t uart_rx_queue; 

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
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(uart_rx_queue, &byte, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void parser_task(void *arg){
    uint8_t byte;

    parser_init(&parser);
    hal_uart1_send_str("parser init\r\n");
    while (1){
        if(xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)){
            if (parse_byte(&parser, byte, &pkt)) {
        
                hal_uart1_send_str("\r\n[OK] Packet received\r\n");
       
                //hal_uart1_send_str("Payload: ");

                //for (uint8_t i = 0; i < pkt.payload_len; i++) {
                    //send_hex_byte(pkt.payload[i]);
                //}

           // hal_uart1_send_str("wait next packet\r\n> ");
            }
        }
    }
    
}

static StackType_t parser_stack[512];
static StaticTask_t parser_tcb;

void fw_init(void) {
    // UART debug
    hal_uart1_init(115200);

    hal_uart1_send_str("\r\n==============================\r\n");
    hal_uart1_send_str("(Firmeware Free RTOS)\r\n");
    hal_uart1_send_str("==============================\r\n");
    hal_uart1_send_str("Send raw bytes via UART2\r\n");
    hal_uart1_send_str("> ");



    uart_rx_queue = xQueueCreateStatic(
        UART_QUEUE_LENGTH,
        sizeof(uint8_t),
        uart_queue_storage,
        &uart_queue_struct
    );
    
    xTaskCreateStatic(
        parser_task,
        "parser",
        512,
        NULL,
        2,
        parser_stack,
        &parser_tcb
    );

    hal_uart2_init(115200, uart2_rx_callback);

}
void fw_run(void) {
    vTaskStartScheduler();

    while (1);
    
}


/* Functions required by FreeRTOS for Static Allocation*/
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, 
                                    StackType_t **ppxIdleTaskStackBuffer, 
                                    uint32_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
                                     StackType_t **ppxTimerTaskStackBuffer, 
                                     uint32_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}