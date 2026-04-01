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
void fw_init(void) {
    /* TODO: Implement initialization */
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
    /* TODO: Start FreeRTOS scheduler */
}
