#ifndef SHELL_H
#define SHELL_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/**
 * @file    shell.h
 * @brief   Interactive Shell Service Module.
 * @details Provides a command-line interface (CLI) to interact with the 
 * firmware via UART. It handles user commands and system notifications.
 */

/* --- Global Handles --- */

/**
 * @brief Global handle for the Shell Task.
 * @note  Used by external callbacks (e.g., GPIO Interrupts) to send 
 * Direct-to-Task notifications for specific events like 'DUMP'.
 */
extern TaskHandle_t xSHELLTaskHandle;

/**
 * @brief Global handle for the Shell RX Queue.
 * @note  Used by the UART ISR to forward received characters to the Shell Task.
 */
extern QueueHandle_t shell_rx_queue;

/* --- Exported Functions --- */

/**
 * @brief  Initialize Shell internal resources.
 * @details Sets up software state machines or local variables required 
 * before the task starts executing.
 */
void shell_init_module(void);

/**
 * @brief   Main Shell Processing Task.
 * @details Blocks waiting for characters from shell_rx_queue or task 
 * notifications. It parses strings and executes registered commands.
 * @param   arg: Task parameters (unused).
 */
void vShellTask(void *arg);

#endif /* SHELL_H */