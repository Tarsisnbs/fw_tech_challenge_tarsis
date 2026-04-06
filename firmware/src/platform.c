#include "platform.h"

/**
 * @file    platform.c
 * @brief   Low-level Hardware Abstraction Layer for ARM Cortex-M.
 * @details Direct register manipulation for the Nested Vectored Interrupt 
 * Controller (NVIC) to ensure RTOS-compatible interrupt behavior.
 */

/** * @brief NVIC Interrupt Priority Register Base Address.
 * @note  Address found in ARM Cortex-M4 Technical Reference Manual.
 * The 'volatile' keyword prevents compiler optimization on hardware access.
 */
#define NVIC_IPR_BASE   ((volatile uint8_t *)0xE000E400UL)

/**
 * @brief  Configures the hardware priority for a specific IRQ.
 * @details STM32F4xx implements only the 4 most significant bits (MSB) 
 * for priority levels. Shifting the value ensures correct alignment 
 * within the 8-bit priority slot.
 * @param  irqn: Interrupt number (e.g., 38 for USART2).
 * @param  priority: Logical priority level.
 */
void platform_set_irq_priority(uint8_t irqn, platform_prio_t priority) {
    /**
     * Internal Logic: Access the specific byte for the IRQ and shift the 
     * 4-bit priority value to the implemented MSB positions.
     */
    NVIC_IPR_BASE[irqn] = (uint8_t)(priority << 4);
}

/**
 * @brief  Enables a specific interrupt in the NVIC.
 * @details Wraps the CMSIS-standard function to maintain abstraction 
 * layers while ensuring the peripheral can trigger the processor.
 * @param  irqn: Interrupt number to be enabled.
 */
void platform_enable_interrupt(uint8_t irqn) {
    /* Utilizing CMSIS standard call for portability and reliability */
    NVIC_EnableIRQ(irqn);
}