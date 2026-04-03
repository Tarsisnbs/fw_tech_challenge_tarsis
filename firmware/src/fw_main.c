
#include "fw_main.h"
#include "hal_uart.h"
#include "hal_gpio.h"
#include "parser.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stddef.h>
#include <stdint.h>


/* --- Static Settings--- */
#define UART_QUEUE_LENGTH 64
#define STACK_SIZE_WORDS  128 

static uint8_t          uart_queue_storage[UART_QUEUE_LENGTH];
static StaticQueue_t    uart_queue_struct;
static QueueHandle_t uart_rx_queue = NULL;

// Task Watchdog
#define STACK_SIZE_LED 128

TaskHandle_t xLEDTaskHandle = NULL;
static StackType_t xLEDStack[STACK_SIZE_LED];
static StaticTask_t xLEDTaskBuffer;

// Task Parser (In this step, acting as Eco/Monitor)
TaskHandle_t xPARSERTaskHandle = NULL;
static StackType_t      parser_stack[STACK_SIZE_WORDS];
static StaticTask_t     parser_tcb;
static parser_t parser;
static raw_packet_t pkt;


/* --- Help Functions--- */

static void send_hex_byte(uint8_t byte) {
    const char hex_digits[] = "0123456789ABCDEF";
    char buffer[3];
    
    buffer[0] = hex_digits[(byte >> 4) & 0x0F];
    buffer[1] = hex_digits[byte & 0x0F];
    buffer[2] = '\0';

    hal_uart1_send_str(buffer);
    hal_uart1_send_str(" "); 
}


/* --- Callbacks and Tasks --- */

static void uart2_rx_callback(uint8_t byte) {
    /*
    hal_uart1_send_str("HW OK: ");
    // Envia o byte de volta para o terminal de debug
    uint8_t buf[1] = {byte};
    hal_uart1_send(buf, 1);
    hal_uart1_send_str("\r\n");
    */
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (uart_rx_queue != NULL) {
        // Send the raw byte to the queue.
        xQueueSendFromISR(uart_rx_queue, &byte, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
}

void vLEDTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;) {
        hal_gpio_pa5_toggle(); 
        // Block the task for 500ms.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

static void parser_task(void *arg) {
    uint8_t byte;

    parser_init(&parser);
    hal_uart1_send_str("parser init\r\n");

    while (1) {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)) {
            if (parse_byte(&parser, byte, &pkt)) {
                hal_uart1_send_str("\r\n[OK] Packet received\r\n");
            }
        }
    }
}

/*
static void parser_task(void *arg) {
    uint8_t byte;
    hal_uart1_send_str("[STEP 2] UART2 Monitor Active...\r\n");
    while (1) {
        // The task sleeps here until the ISR wakes it up with a byte.
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)) {
            
            // TEST LOG: If you see this, the interrupt and the queue are OK.
            hal_uart1_send_str(" ISR ok");

            // The Watchdog LED should continue blinking normally.
        }
    }
}
*/
void fw_init(void) {
    hal_gpio_pa5_init();

    hal_uart1_init(115200);
    hal_uart1_send_str("\r\n--- STARTING LAYER 2 TEST ---\r\n");
 
    //Create Queue
    uart_rx_queue = xQueueCreateStatic(
        UART_QUEUE_LENGTH, 1, 
        uart_queue_storage, &uart_queue_struct
    );

    //Creating the Task statically
    xLEDTaskHandle = xTaskCreateStatic(
        vLEDTask,           
        "LED_TASK",         
        STACK_SIZE_LED,   
        NULL,               
        tskIDLE_PRIORITY + 2,                  
        xLEDStack,         
        &xLEDTaskBuffer     
    );
    if (xLEDTaskHandle == NULL){
        hal_uart1_send_str("ERROR: Failed to create LED task\r\n");
        while(1);
    }
    xPARSERTaskHandle = xTaskCreateStatic(
        parser_task, 
        "echo", 
        STACK_SIZE_WORDS, 
        NULL, 
        1, 
        parser_stack, 
        &parser_tcb
    );
    hal_uart2_init(115200, uart2_rx_callback);
    /* RTOS COMPATIBILITY REPAIR (Invisible to the HAL) Address 0xE000E400 
    is the beginning of the Cortex-M4 priority registers (IPR). USART2 
    on the STM32F411 is interrupt number 38. */
    volatile uint8_t *nvic_ipr = (volatile uint8_t *)0xE000E400;
    nvic_ipr[38] = (uint8_t)(7 << 4); // Define prioridade 7 (Segura para FreeRTOS)
}

void fw_run(void) {
    
    //Start the scheduler.
    vTaskStartScheduler();
    hal_uart1_send_str("ERROR: The scheduler has stopped.\r\n");
    while(1){

    }
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