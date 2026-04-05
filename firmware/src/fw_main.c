
#include "fw_main.h"
#include "hal_uart.h"
#include "hal_gpio.h"
#include "parser.h"
#include "platform.h"
#include "storage.h"  
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <string.h>

/* --- Static Settings--- */
#define UART_QUEUE_LENGTH 128
#define STACK_SIZE_WORDS  256

static uint8_t          uart_queue_storage[UART_QUEUE_LENGTH];
static StaticQueue_t    uart_queue_struct;
static QueueHandle_t    uart_rx_queue = NULL;

// Task LED
#define STACK_SIZE_LED 128
static StackType_t      xLEDStack[STACK_SIZE_LED];
static StaticTask_t     xLEDTaskBuffer;
TaskHandle_t xLEDTaskHandle = NULL;

// Task Parser
static StackType_t      parser_stack[STACK_SIZE_WORDS];
static StaticTask_t     parser_tcb;
static parser_t         parser;
static raw_packet_t     pkt;
TaskHandle_t xPARSERTaskHandle = NULL;

//Telemetry global Variables
volatile uint32_t       isr_drop_count = 0; 
volatile uint32_t       isr_rx_count = 0;

static void uart2_rx_callback(uint8_t byte) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (uart_rx_queue != NULL) {
        // Send the raw byte to the queue.
       if(xQueueSendFromISR(uart_rx_queue, &byte, &xHigherPriorityTaskWoken) != pdPASS){
            isr_drop_count++;
       }
        else {
            isr_rx_count++;
        }
    }
    
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
}

void vLEDTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;) {
        hal_gpio_pa5_toggle(); 
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

static void parser_task(void *arg) {
    uint8_t byte;
    parser_init(&parser);
    hal_uart1_send_str("Parser Task: Running\r\n");

    for (;;) {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)) {
            if (parse_byte(&parser, byte, &pkt)) {
                //The storage handles the Mutex and the Ring Buffer.
                //Light debug here
                if (storage_save_packet(pkt.sensor_id, pkt.payload, pkt.payload_len)) {
                    hal_uart1_send_str("."); //save ok in buffer
                }
                else{
                    hal_uart1_send_str("!"); //save error
                }
                memset(&pkt, 0, sizeof(raw_packet_t));
            }
        }
    }
}

void fw_init(void) {
    //Basic Initialization
    hal_gpio_pa5_init();
    hal_uart1_init(115200);
    hal_uart1_send_str("\r\n--- STARTING LAYER 2 (parser) and LAYER 3 (ringbuffer) TEST ---\r\n");
    
    //Kernel and Data Module Initialization
    storage_init();
    uart_rx_queue = xQueueCreateStatic(
        UART_QUEUE_LENGTH, 1, 
        uart_queue_storage, &uart_queue_struct
    );

    //Hardware and NVIC configuration (via Platform)
    platform_set_irq_priority(USART2_IRQn, PLATFORM_PRIO_SAFE);
    platform_enable_interrupt(USART2_IRQn);
    hal_uart2_init(115200, uart2_rx_callback);

    //Creating Tasks
    xLEDTaskHandle = xTaskCreateStatic(
        vLEDTask,           
        "LED_TASK",         
        STACK_SIZE_LED,   
        NULL,               
        tskIDLE_PRIORITY + 3,                  
        xLEDStack,         
        &xLEDTaskBuffer     
    );
    if (xLEDTaskHandle == NULL){
        hal_uart1_send_str("ERROR: Failed to create LED task\r\n");
        while(1);
    }
    xPARSERTaskHandle = xTaskCreateStatic(
        parser_task, 
        "parser", 
        STACK_SIZE_WORDS, 
        NULL, 
        1, 
        parser_stack, 
        &parser_tcb
    );
    
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