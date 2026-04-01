/* DO NOT MODIFY */
#include "hal_uart.h"
#include "stm32f411xe.h"
#include <string.h>

static uart_rx_cb_t uart2_rx_callback = NULL;

void hal_uart2_init(uint32_t baud, uart_rx_cb_t on_rx_byte) {
    uart2_rx_callback = on_rx_byte;

    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* Configure PA2 (TX), PA3 (RX) as alternate function */
    GPIOA->MODER &= ~((3UL << (2 * 2)) | (3UL << (2 * 3)));
    GPIOA->MODER |= (GPIO_MODER_AF << (2 * 2)) | (GPIO_MODER_AF << (2 * 3));
    GPIOA->AFR[0] &= ~((0xFUL << (4 * 2)) | (0xFUL << (4 * 3)));
    GPIOA->AFR[0] |= (7UL << (4 * 2)) | (7UL << (4 * 3));  /* AF7 = USART2 */

    /* Configure UART: 8N1 */
    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

    /* Baud rate: assuming 16 MHz APB1 clock */
    USART2->BRR = (16000000UL + baud / 2) / baud;

    /* Enable UART, RX, and RXNE interrupt */
    USART2->CR1 |= USART_CR1_UE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART2 interrupt in NVIC */
    NVIC_EnableIRQ(USART2_IRQn);
}

void hal_uart1_init(uint32_t baud) {
    /* Enable clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* Configure PA9 (TX), PA10 (RX) as alternate function */
    GPIOA->MODER &= ~((3UL << (2 * 9)) | (3UL << (2 * 10)));
    GPIOA->MODER |= (GPIO_MODER_AF << (2 * 9)) | (GPIO_MODER_AF << (2 * 10));
    GPIOA->AFR[1] &= ~((0xFUL << (4 * 1)) | (0xFUL << (4 * 2)));
    GPIOA->AFR[1] |= (7UL << (4 * 1)) | (7UL << (4 * 2));  /* AF7 = USART1 */

    /* Configure UART: 8N1 */
    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    /* Baud rate: assuming 16 MHz APB2 clock */
    USART1->BRR = (16000000UL + baud / 2) / baud;

    /* Enable UART, TX, RX */
    USART1->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

void hal_uart1_send(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        while (!(USART1->SR & USART_SR_TXE));
        USART1->DR = buf[i];
    }
    /* Wait for transmission complete */
    while (!(USART1->SR & USART_SR_TC));
}

void hal_uart1_send_str(const char *str) {
    hal_uart1_send((const uint8_t *)str, strlen(str));
}

/* USART2 IRQ Handler */
void USART2_IRQHandler(void) {
    if (USART2->SR & USART_SR_RXNE) {
        uint8_t byte = (uint8_t)(USART2->DR & 0xFF);
        if (uart2_rx_callback) {
            uart2_rx_callback(byte);
        }
    }
}
