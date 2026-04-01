/* DO NOT MODIFY */
#ifndef FW_MAIN_H
#define FW_MAIN_H

/**
 * @brief Firmware initialization
 *
 * Called once from main() before the scheduler starts.
 * Use this to initialize HAL, create FreeRTOS objects (tasks, queues, mutexes),
 * and perform any other one-time setup.
 *
 * MUST use static allocation for all FreeRTOS objects:
 * - xTaskCreateStatic()
 * - xQueueCreateStatic()
 * - xSemaphoreCreateMutexStatic()
 */
void fw_init(void);

/**
 * @brief Firmware main loop
 *
 * Called once from main() after fw_init().
 * MUST call vTaskStartScheduler() - this function should never return.
 */
void fw_run(void);

#endif /* FW_MAIN_H */
