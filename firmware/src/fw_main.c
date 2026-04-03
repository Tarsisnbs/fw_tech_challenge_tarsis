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


TaskHandle_t xLEDTaskHandle = NULL;

/* ================= WATCHDOG LED TASK ================= 
         128 words = 512 bytes (requirement limit)*/
#define STACK_SIZE_LED 128
static StackType_t xLEDStack[STACK_SIZE_LED];
static StaticTask_t xLEDTaskBuffer;
void vLEDTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for(;;) {
        hal_gpio_pa5_toggle(); 
        // Block the task for 500ms.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
    }
}

void fw_init(void) {
    hal_gpio_pa5_init();

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