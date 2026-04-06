
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
#include "sensors.h"

/* --- Static Settings--- */
#define UART_QUEUE_LENGTH 128
#define STACK_SIZE_WORDS  256

static uint8_t          uart_queue_storage[UART_QUEUE_LENGTH];
static StaticQueue_t    uart_queue_struct;
static QueueHandle_t    uart_rx_queue = NULL;

/* --- Shell Infrastructure --- */
static uint8_t          shell_queue_storage[32];
static StaticQueue_t    shell_queue_struct;
QueueHandle_t           shell_rx_queue = NULL; // Sem static para o shell.c enxergar

#define STACK_SIZE_SHELL 256
static StackType_t      shell_stack[STACK_SIZE_SHELL];
static StaticTask_t     shell_tcb;
TaskHandle_t            xSHELLTaskHandle = NULL;

// Task WDGLED
#define STACK_SIZE_LED 128
static StackType_t      xLEDStack[STACK_SIZE_LED];
static StaticTask_t     xLEDTaskBuffer;
TaskHandle_t xWDGLEDTaskHandle = NULL;

// Task PA6LED
#define STACK_SIZE_PA6LED 128
static StackType_t      xFaultMonitorstack[STACK_SIZE_PA6LED];
static StaticTask_t     xFaultMonitorskBuffer;
TaskHandle_t xFAULT_MONHandle = NULL;

// Task Parser
static StackType_t      parser_stack[STACK_SIZE_WORDS];
static StaticTask_t     parser_tcb;
static parser_t         parser;
static raw_packet_t     pkt;
TaskHandle_t xPARSERTaskHandle = NULL;

//Telemetry global Variables
volatile uint32_t       isr_drop_count = 0; 
volatile uint32_t       isr_rx_count = 0;

//acess USART1
static StaticSemaphore_t uart1_mutex_buffer;
static SemaphoreHandle_t uart1_mutex;
//////////////////////////////////////////////////////////shell.h
typedef void (*shell_print_fn)(const char *str);
// Struct de configuração que você passa no shell_init
typedef struct {
    shell_print_fn print;
} shell_config_t;
void shell_init(shell_config_t *config);
void vShellTask(void *argument);

//////////////////////////////////////////////////////////SHEL.C
extern QueueHandle_t shell_rx_queue;
static shell_config_t shell_cfg;

void shell_init(shell_config_t *config) {
    if (config) shell_cfg = *config;
}

static void process_command(char *cmd) {
    char out[64];
    if (strcmp(cmd, "STATUS") == 0) {
        int n = 0;
        for (int i = 0; i < MAX_SENSORS; i++) {
            if (storage_get_sensor(i)->registered) n++;
        }
        snprintf(out, sizeof(out), "OK sensors=%d fault=%d\n", n, sensors_check_for_timeout(2000));
        shell_cfg.print(out);
    } 
    else if (strcmp(cmd, "DUMP") == 0) {
        if (storage_lock(pdMS_TO_TICKS(100))) {
            for (int i = 0; i < MAX_SENSORS; i++) {
                sensor_data_t *s = storage_get_sensor(i);
                if (s && s->registered) {
                    uint16_t hex = (s->last_payload[0] << 8) | s->last_payload[1];
                    snprintf(out, sizeof(out), "%d last=%04X count=%lu\n", i, hex, s->sample_count);
                    shell_cfg.print(out);
                }
            }
            shell_cfg.print("END\n");
            storage_unlock();
        }
    }
    else if (strcmp(cmd, "RESET") == 0) {
        storage_init();
        shell_cfg.print("OK\n");
    }
    else if (strcmp(cmd, "?") == 0) {
        shell_cfg.print("Commands: STATUS, DUMP, RESET, ?\nEND\n");
    }
    else if (strlen(cmd) > 0) {
        shell_cfg.print("ERR unknown\n");
    }
}

void vShellTask(void *argument) {
    (void)argument;
    uint8_t c;
    static char buf[32];
    static uint8_t idx = 0;

    shell_cfg.print("Shell Online\n");

    for (;;) {
        // Se o botão for apertado, xTaskNotifyGive acorda isso aqui
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            process_command("DUMP");
        }

        if (xQueueReceive(shell_rx_queue, &c, pdMS_TO_TICKS(50)) == pdPASS) {
            if (c == '\n' || c == '\r') {
                buf[idx] = '\0';
                process_command(buf);
                idx = 0;
            } else if (idx < 31) {
                buf[idx++] = c;
            }
        }
    }
}
//////////////////////////////////////////////////////////SHEL.C
// Handler manual para capturar comandos do terminal
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t byte = (uint8_t)(USART1->DR & 0xFF);
        BaseType_t woken = pdFALSE;
        if (shell_rx_queue != NULL) {
            xQueueSendFromISR(shell_rx_queue, &byte, &woken);
        }
        portYIELD_FROM_ISR(woken);
    }
}

// Callback do botão para o Requisito 6
void pb0_falling_callback(void) {
    if (xSHELLTaskHandle != NULL) {
        xTaskNotifyGive(xSHELLTaskHandle);
    }
}

//secure use of USART1 with MUTEX
static void uart1_safe_send(const char *str) { 
    if (uart1_mutex) {
        xSemaphoreTake(uart1_mutex, portMAX_DELAY);
        hal_uart1_send_str(str);
        xSemaphoreGive(uart1_mutex);
    }
}


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

void vWDGLEDTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;) {
        hal_gpio_pa5_toggle(); 
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

static void parser_task(void *arg) {
    uint8_t byte;
    parser_init(&parser);
    uart1_safe_send("Parser Task: Running\r\n");

    for (;;) {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)) {
            //Layer 2 - Attempts to assemble the package.
            if (parse_byte(&parser, byte, &pkt)) {
                //Layer 4 - Indicates that the sensor has given a sign of life.
                //          It handles the lock, tick, and registered operations internally.
                sensors_touch_alive(pkt.sensor_id);

                // Layer 3 - Save the data to storage.
                //           the '.' indicate that te data was stored.
                if (storage_save_packet(pkt.sensor_id, pkt.payload, pkt.payload_len)) {
                    uart1_safe_send("."); 
                } else {
                    uart1_safe_send("!"); 
                }
                //Clean up for next time
                memset(&pkt, 0, sizeof(raw_packet_t));
            }
        }
    }
}

void vFaultMonitorTask(void *pvParameters) {
    while(1) {
        // Layer 4 is ok
        bool system_fault = sensors_check_for_timeout(2000);
        hal_gpio_pa6_set(system_fault);   
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

//lad task 
void led_pa6_task(void *arg) {
    while (1) {
        hal_gpio_pa6_toggle();
        vTaskDelay(pdMS_TO_TICKS(500)); // 500ms
    }
}

void fw_init(void) {
    //Basic Initialization
    hal_gpio_pa5_init();
    hal_gpio_pa6_init();
    hal_uart1_init(115200);
    hal_gpio_pb0_init(pb0_falling_callback);
    platform_set_irq_priority(EXTI0_IRQn, 10); 
    
    platform_enable_interrupt(EXTI0_IRQn);
    uart1_mutex = xSemaphoreCreateMutexStatic(&uart1_mutex_buffer);
    //Inicia Fila do Shell e Interrupção
    shell_config_t s_cfg = { .print = uart1_safe_send };
    shell_init(&s_cfg);

    shell_rx_queue = xQueueCreateStatic(32, 1, shell_queue_storage, &shell_queue_struct);
    USART1->CR1 |= USART_CR1_RXNEIE; 
    platform_set_irq_priority(USART1_IRQn, PLATFORM_PRIO_SAFE);
    platform_enable_interrupt(USART1_IRQn);

    uart1_safe_send("\r\n--- STARTING L2(Parser), L3(Storage) and L4(Sensor Management) ---\r\n");
    
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
    xWDGLEDTaskHandle = xTaskCreateStatic(
        vWDGLEDTask,           
        "LED_TASK",         
        STACK_SIZE_LED,   
        NULL,               
        tskIDLE_PRIORITY + 3,                  
        xLEDStack,         
        &xLEDTaskBuffer     
    );
    if (xWDGLEDTaskHandle == NULL){
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
    xFAULT_MONHandle = xTaskCreateStatic(
        vFaultMonitorTask,
        "FAULT_MON", 
        128,
        NULL, 
        2, 
        xFaultMonitorstack,      
        &xFaultMonitorskBuffer
    );  
    xSHELLTaskHandle = xTaskCreateStatic(
        vShellTask, 
        "SHELL", 
        STACK_SIZE_SHELL, 
        NULL, 
        1, 
        shell_stack, 
        &shell_tcb);
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