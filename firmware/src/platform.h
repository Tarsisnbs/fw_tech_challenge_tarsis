#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "stm32f411xe.h" 

// Priority Abstraction for FreeRTOS
typedef enum {
    PLATFORM_PRIO_LOW    = 15,
    PLATFORM_PRIO_SAFE   = 10, 
    PLATFORM_PRIO_MEDIUM = 7,
    PLATFORM_PRIO_HIGH   = 2
} platform_prio_t;


void platform_set_irq_priority(uint8_t irqn, platform_prio_t priority);
void platform_enable_interrupt(uint8_t irqn);

#endif