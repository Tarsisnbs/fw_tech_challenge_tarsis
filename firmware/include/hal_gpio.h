/* DO NOT MODIFY */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdbool.h>

/**
 * @brief GPIO interrupt callback function type
 * @note Called from ISR context - keep processing minimal
 */
typedef void (*gpio_irq_cb_t)(void);

/**
 * @brief Initialize PA5 as output (Watchdog LED)
 */
void hal_gpio_pa5_init(void);

/**
 * @brief Set PA5 output state
 * @param state true = HIGH, false = LOW
 */
void hal_gpio_pa5_set(bool state);

/**
 * @brief Toggle PA5 output state
 */
void hal_gpio_pa5_toggle(void);

/**
 * @brief Initialize PA6 as output (Fault indicator)
 */
void hal_gpio_pa6_init(void);

/**
 * @brief Set PA6 output state
 * @param active true = fault active (HIGH), false = no fault (LOW)
 */
void hal_gpio_pa6_set(bool active);

/**
 * @brief Initialize PB0 as input with falling-edge interrupt
 * @param on_falling_edge Callback invoked on falling edge (ISR context)
 */
void hal_gpio_pb0_init(gpio_irq_cb_t on_falling_edge);

#endif /* HAL_GPIO_H */
