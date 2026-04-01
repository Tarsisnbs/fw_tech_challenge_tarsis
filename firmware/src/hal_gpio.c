/* DO NOT MODIFY */
#include "hal_gpio.h"
#include "stm32f411xe.h"
#include <stddef.h>

static gpio_irq_cb_t pb0_callback = NULL;

void hal_gpio_pa5_init(void) {
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Configure PA5 as output */
    GPIOA->MODER &= ~(3UL << (2 * 5));
    GPIOA->MODER |= (GPIO_MODER_OUTPUT << (2 * 5));

    /* Set to push-pull, low speed */
    GPIOA->OTYPER &= ~(1UL << 5);
    GPIOA->OSPEEDR &= ~(3UL << (2 * 5));

    /* No pull-up/pull-down */
    GPIOA->PUPDR &= ~(3UL << (2 * 5));

    /* Initial state: LOW */
    GPIOA->BSRR = (1UL << (5 + 16));
}

void hal_gpio_pa5_set(bool state) {
    if (state) {
        GPIOA->BSRR = (1UL << 5);  /* Set */
    } else {
        GPIOA->BSRR = (1UL << (5 + 16));  /* Reset */
    }
}

void hal_gpio_pa5_toggle(void) {
    GPIOA->ODR ^= (1UL << 5);
}

void hal_gpio_pa6_init(void) {
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Configure PA6 as output */
    GPIOA->MODER &= ~(3UL << (2 * 6));
    GPIOA->MODER |= (GPIO_MODER_OUTPUT << (2 * 6));

    /* Set to push-pull, low speed */
    GPIOA->OTYPER &= ~(1UL << 6);
    GPIOA->OSPEEDR &= ~(3UL << (2 * 6));

    /* No pull-up/pull-down */
    GPIOA->PUPDR &= ~(3UL << (2 * 6));

    /* Initial state: LOW (no fault) */
    GPIOA->BSRR = (1UL << (6 + 16));
}

void hal_gpio_pa6_set(bool active) {
    if (active) {
        GPIOA->BSRR = (1UL << 6);  /* Set */
    } else {
        GPIOA->BSRR = (1UL << (6 + 16));  /* Reset */
    }
}

void hal_gpio_pb0_init(gpio_irq_cb_t on_falling_edge) {
    pb0_callback = on_falling_edge;

    /* Enable GPIOB clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    /* Configure PB0 as input */
    GPIOB->MODER &= ~(3UL << (2 * 0));  /* Input mode */

    /* Pull-up */
    GPIOB->PUPDR &= ~(3UL << (2 * 0));
    GPIOB->PUPDR |= (1UL << (2 * 0));  /* Pull-up */

    /* Enable SYSCFG clock for EXTI */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    /* Connect PB0 to EXTI0 */
    SYSCFG->EXTICR[0] &= ~(0xFUL << 0);
    SYSCFG->EXTICR[0] |= (1UL << 0);  /* EXTI0 = PB */

    /* Configure EXTI0 for falling edge */
    EXTI->IMR |= (1UL << 0);   /* Unmask EXTI0 */
    EXTI->FTSR |= (1UL << 0);  /* Falling edge trigger */
    EXTI->RTSR &= ~(1UL << 0); /* Disable rising edge */

    /* Enable EXTI0 interrupt in NVIC */
    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* EXTI0 IRQ Handler */
void EXTI0_IRQHandler(void) {
    if (EXTI->PR & (1UL << 0)) {
        EXTI->PR = (1UL << 0);  /* Clear pending flag */
        if (pb0_callback) {
            pb0_callback();
        }
    }
}
