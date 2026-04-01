/* DO NOT MODIFY */
#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief UART RX callback function type
 * @note Called from ISR context - keep processing minimal
 */
typedef void (*uart_rx_cb_t)(uint8_t byte);

/**
 * @brief Initialize UART2 with RX interrupt callback
 * @param baud Baud rate (e.g., 115200)
 * @param on_rx_byte Callback invoked for each received byte (ISR context)
 */
void hal_uart2_init(uint32_t baud, uart_rx_cb_t on_rx_byte);

/**
 * @brief Initialize UART1 for TX/RX (diagnostic shell)
 * @param baud Baud rate (e.g., 115200)
 */
void hal_uart1_init(uint32_t baud);

/**
 * @brief Send buffer over UART1 (blocking)
 * @param buf Data buffer
 * @param len Number of bytes to send
 */
void hal_uart1_send(const uint8_t *buf, size_t len);

/**
 * @brief Send null-terminated string over UART1 (blocking)
 * @param str Null-terminated string
 */
void hal_uart1_send_str(const char *str);

#endif /* HAL_UART_H */
