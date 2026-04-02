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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


#define STACK_SIZE_LED 128
static StackType_t xLEDStack[STACK_SIZE_LED];
static StaticTask_t xLEDTaskBuffer;
TaskHandle_t xLEDTaskHandle = NULL;

void vLEDTask(void *pvParameters) {
    /* Configuração inicial do GPIO se necessário */
    for(;;) {
        // Altera o estado do LED (verifique o nome correto na sua HAL)
        hal_gpio_pa5_toggle(); 
        
        // Bloqueia a task por 500ms
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}


void fw_init(void) {
    /* 1. Inicialização do Hardware (Clock, GPIO, etc) */
    hal_gpio_pa5_init();

    /* 2. Criação da Task de forma estática */
    xLEDTaskHandle = xTaskCreateStatic(
        vLEDTask,           /* Função da task */
        "LED_TASK",         /* Nome para debug */
        STACK_SIZE_LED,     /* Tamanho da pilha */
        NULL,               /* Parâmetros */
        1,                  /* Prioridade */
        xLEDStack,          /* Array da pilha */
        &xLEDTaskBuffer     /* Buffer de controle */
    );

}

/**
 * @brief Firmware main loop
 *
 * Called once from main() after fw_init().
 *
 * TODO: Implement the following:
 * 1. Call vTaskStartScheduler() to start the FreeRTOS scheduler
 * 2. This function should NEVER return (scheduler runs forever)
 * 3. If the scheduler returns (should never happen), enter an error state
 */
void fw_run(void) {
    
    /* 3. Inicia o escalonador */
    vTaskStartScheduler();
    hal_uart1_send_str("ERRO: O escalonador parou!\r\n");
    while(1){

    }
}

/* Funções exigidas pelo FreeRTOS para Alocação Estática */
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