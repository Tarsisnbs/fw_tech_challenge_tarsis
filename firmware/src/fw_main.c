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
#include <stdio.h>
#include "sensors.h"

/* --- Infrastructure & Queues --- */
#define UART_QUEUE_LENGTH 128
#define STACK_SIZE_WORDS  256
static uint8_t          uart_queue_storage[UART_QUEUE_LENGTH];
static StaticQueue_t    uart_queue_struct;
static QueueHandle_t    uart_rx_queue = NULL;

static uint8_t          shell_queue_storage[32];
static StaticQueue_t    shell_queue_struct;
QueueHandle_t           shell_rx_queue = NULL; 

/* --- Task Handles & Buffers --- */
TaskHandle_t xSHELLTaskHandle = NULL;
static StaticTask_t shell_tcb;
static StackType_t  shell_stack[256];

TaskHandle_t xWDGLEDTaskHandle = NULL;
static StaticTask_t xLEDTaskBuffer;
static StackType_t  xLEDStack[128];

TaskHandle_t xFAULT_MONHandle = NULL;
static StaticTask_t xFaultMonitorskBuffer;
static StackType_t  xFaultMonitorstack[128];

TaskHandle_t xPARSERTaskHandle = NULL;
static StaticTask_t parser_tcb;
static StackType_t  parser_stack[STACK_SIZE_WORDS];

/* --- Global Resources --- */
static parser_t         parser;
static raw_packet_t     pkt;
static StaticSemaphore_t uart1_mutex_buffer;
static SemaphoreHandle_t uart1_mutex;

/* --- Shell Logic (Injection) --- */
typedef void (*shell_print_fn)(const char *str);
typedef struct { shell_print_fn print; } shell_config_t;
static shell_config_t shell_cfg;

void shell_init(shell_config_t *config) { if (config) shell_cfg = *config; }

/* --- UART 1 Safe Send (Mutex Protected) --- */
static void uart1_safe_send(const char *str) { 
    if (uart1_mutex) {
        xSemaphoreTake(uart1_mutex, portMAX_DELAY);
        hal_uart1_send_str(str);
        xSemaphoreGive(uart1_mutex);
    } else {
        // Se o mutex ainda não existir (bem no início), manda direto
        hal_uart1_send_str(str);
    }
}

/* --- Command Processor --- */
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
}

/* --- INTERRUPT HANDLERS --- */

void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t byte = (uint8_t)(USART1->DR & 0xFF);
        BaseType_t woken = pdFALSE;
        if (shell_rx_queue != NULL) xQueueSendFromISR(shell_rx_queue, &byte, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void pb0_falling_callback(void) {
    // Como esta função é chamada DE DENTRO da interrupção da HAL...
    if (xSHELLTaskHandle != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        // Usamos a versão FromISR porque estamos em contexto de interrupção
        vTaskNotifyGiveFromISR(xSHELLTaskHandle, &xHigherPriorityTaskWoken);
        
        // Solicita troca de contexto se necessário
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void uart2_rx_callback(uint8_t byte) {
    BaseType_t woken = pdFALSE;
    if (uart_rx_queue != NULL) xQueueSendFromISR(uart_rx_queue, &byte, &woken);
    portYIELD_FROM_ISR(woken);
}

/* --- TASKS --- */

void vShellTask(void *arg) {
    (void)arg;
    uint8_t c;
    static char buf[32];
    static uint8_t idx = 0;
    shell_cfg.print("Shell Online\n");
    for (;;) {
        // Acorda se o botão for pressionado OU se chegar comando via UART
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            process_command("DUMP");
        }
        if (xQueueReceive(shell_rx_queue, &c, pdMS_TO_TICKS(50)) == pdPASS) {
            if (c == '\n' || c == '\r') {
                buf[idx] = '\0';
                process_command(buf);
                idx = 0;
            } else if (idx < 31) buf[idx++] = c;
        }
    }
}

static void parser_task(void *arg) {
    (void)arg;
    uint8_t byte;
    parser_init(&parser);
    for (;;) {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)) {
            if (parse_byte(&parser, byte, &pkt)) {
                sensors_touch_alive(pkt.sensor_id);
                if (storage_save_packet(pkt.sensor_id, pkt.payload, pkt.payload_len)) {
                    uart1_safe_send("."); 
                }
                memset(&pkt, 0, sizeof(raw_packet_t));
            }
        }
    }
}

void vWDGLEDTask(void *pvParameters) {
    (void)pvParameters;
    for(;;) {
        hal_gpio_pa5_toggle(); 
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vFaultMonitorTask(void *pvParameters) {
    (void)pvParameters;
    for(;;) {
        bool system_fault = sensors_check_for_timeout(2000);
        hal_gpio_pa6_set(system_fault);   
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* --- CORE INIT --- */

void fw_init(void) {
// 1. Inicialização de Hardware (LEDs e UARTs)
    hal_gpio_pa5_init();
    hal_gpio_pa6_init();
    hal_uart1_init(115200);

    // 2. Recursos do Kernel (Sempre antes das Tasks)
    uart1_mutex = xSemaphoreCreateMutexStatic(&uart1_mutex_buffer);
    
    // Configuração do Shell (Injeção do print seguro)
    shell_config_t s_cfg = { .print = uart1_safe_send };
    shell_init(&s_cfg);
    shell_rx_queue = xQueueCreateStatic(32, 1, shell_queue_storage, &shell_queue_struct);

    // 3. Configuração do Botão (PB0) - Requisito 6
    // Passamos o callback que agora tem o vTaskNotifyGiveFromISR
    hal_gpio_pb0_init(pb0_falling_callback);
    
    // Configuração de prioridade segura para o FreeRTOS (Evita o HardFault)
    platform_set_irq_priority(EXTI0_IRQn, 10); 
    platform_enable_interrupt(EXTI0_IRQn);

    // 4. Configuração da UART1 (Shell RX)
    USART1->CR1 |= USART_CR1_RXNEIE; 
    platform_set_irq_priority(USART1_IRQn, PLATFORM_PRIO_SAFE);
    platform_enable_interrupt(USART1_IRQn);

    // 5. Inicialização de Dados e Sensores (UART2)
    storage_init();
    uart_rx_queue = xQueueCreateStatic(UART_QUEUE_LENGTH, 1, uart_queue_storage, &uart_queue_struct);
    
    platform_set_irq_priority(USART2_IRQn, PLATFORM_PRIO_SAFE);
    platform_enable_interrupt(USART2_IRQn);
    hal_uart2_init(115200, uart2_rx_callback);

    // Criação das Tasks
    xWDGLEDTaskHandle = xTaskCreateStatic(vWDGLEDTask, "LED", 128, NULL, 3, xLEDStack, &xLEDTaskBuffer);
    xPARSERTaskHandle = xTaskCreateStatic(parser_task, "PRS", 256, NULL, 1, parser_stack, &parser_tcb);
    xFAULT_MONHandle  = xTaskCreateStatic(vFaultMonitorTask, "FLT", 128, NULL, 2, xFaultMonitorstack, &xFaultMonitorskBuffer);
    xSHELLTaskHandle  = xTaskCreateStatic(vShellTask, "SHL", 256, NULL, 1, shell_stack, &shell_tcb);

    uart1_safe_send("\r\n--- STARTING L2, L3 and L4 ---\r\n");
}

void fw_run(void) {
    vTaskStartScheduler();
    while(1);
}

/* --- Idle & Timer Memory (Required) --- */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB; *ppxIdleTaskStackBuffer = uxIdleTaskStack; *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB; *ppxTimerTaskStackBuffer = uxTimerTaskStack; *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}