#include "platform.h"

// O endereço que você encontrou no manual da ARM
#define NVIC_IPR_BASE   ((volatile uint8_t *)0xE000E400UL)

void platform_set_irq_priority(uint8_t irqn, platform_prio_t priority) {
    // Usamos o endereço base e o índice da interrupção (ex: 38 para USART2)
    // Deslocamos 4 bits para a esquerda porque o STM32 usa apenas os MSB
    NVIC_IPR_BASE[irqn] = (uint8_t)(priority << 4);
}

void platform_enable_interrupt(uint8_t irqn) {
    // Aqui usamos a função static inline que já veio na sua library!
    NVIC_EnableIRQ(irqn);
}