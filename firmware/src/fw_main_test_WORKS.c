/**
 * TEMPORARY TEST FILE - Verifies HAL UART works
 * This initializes UART and echoes characters back
 */

#include "fw_main.h"
#include "hal_uart.h"
#include "hal_gpio.h"

// Simple echo callback
static void uart1_echo(uint8_t byte) {
    hal_uart1_send(&byte, 1);
    if (byte == '\r' || byte == '\n') {
        hal_uart1_send_str("\r\n> ");
    }
}

void fw_init(void) {
    // Initialize UART1 for TX/RX
    hal_uart1_init(115200);

    // Send welcome banner
    hal_uart1_send_str("\r\n");
    hal_uart1_send_str("=================================\r\n");
    hal_uart1_send_str("STM32F411RE HAL UART Test\r\n");
    hal_uart1_send_str("=================================\r\n");
    hal_uart1_send_str("Type anything - it will echo\r\n");
    hal_uart1_send_str("> ");

    // Initialize UART2 with callback
    hal_uart2_init(115200, uart1_echo);

    // Initialize GPIOs
    hal_gpio_pa5_init();
    hal_gpio_pa6_init();
    hal_gpio_pb0_init(NULL);
}

void fw_run(void) {
    // Simple loop with LED toggle
    volatile uint32_t counter = 0;
    while (1) {
        counter++;
        if ((counter % 1000000) == 0) {
            hal_gpio_pa5_toggle();
        }
        for (volatile int i = 0; i < 100; i++);
    }
}
