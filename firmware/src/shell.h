#ifndef SHELL_H
#define SHELL_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Handle global para que o callback do botão (na main ou hal_gpio) consiga notificar
extern TaskHandle_t xSHELLTaskHandle;
extern QueueHandle_t shell_rx_queue;

// Inicializa os recursos do Shell
void shell_init_module(void);

// A Task que processa o Shell
void vShellTask(void *arg);

#endif