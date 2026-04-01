/* DO NOT MODIFY */
/* Minimal STM32F411xE register definitions for HAL implementation */
#ifndef STM32F411XE_H
#define STM32F411XE_H

#include <stdint.h>

/* Base addresses */
#define PERIPH_BASE           0x40000000UL
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE            (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE            (AHB1PERIPH_BASE + 0x0400UL)
#define RCC_BASE              (AHB1PERIPH_BASE + 0x3800UL)
#define USART1_BASE           (APB2PERIPH_BASE + 0x1000UL)
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400UL)
#define SYSCFG_BASE           (APB2PERIPH_BASE + 0x3800UL)
#define EXTI_BASE             (APB2PERIPH_BASE + 0x3C00UL)

/* GPIO registers */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

/* USART registers */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

/* RCC registers */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

/* SYSCFG registers */
typedef struct {
    volatile uint32_t MEMRMP;
    volatile uint32_t PMC;
    volatile uint32_t EXTICR[4];
    uint32_t RESERVED[2];
    volatile uint32_t CMPCR;
} SYSCFG_TypeDef;

/* EXTI registers */
typedef struct {
    volatile uint32_t IMR;
    volatile uint32_t EMR;
    volatile uint32_t RTSR;
    volatile uint32_t FTSR;
    volatile uint32_t SWIER;
    volatile uint32_t PR;
} EXTI_TypeDef;

/* Peripheral instances */
#define GPIOA               ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *)GPIOB_BASE)
#define USART1              ((USART_TypeDef *)USART1_BASE)
#define USART2              ((USART_TypeDef *)USART2_BASE)
#define RCC                 ((RCC_TypeDef *)RCC_BASE)
#define SYSCFG              ((SYSCFG_TypeDef *)SYSCFG_BASE)
#define EXTI                ((EXTI_TypeDef *)EXTI_BASE)

/* RCC register bit definitions */
#define RCC_AHB1ENR_GPIOAEN_Pos   (0U)
#define RCC_AHB1ENR_GPIOAEN       (1UL << RCC_AHB1ENR_GPIOAEN_Pos)
#define RCC_AHB1ENR_GPIOBEN_Pos   (1U)
#define RCC_AHB1ENR_GPIOBEN       (1UL << RCC_AHB1ENR_GPIOBEN_Pos)
#define RCC_APB2ENR_USART1EN_Pos  (4U)
#define RCC_APB2ENR_USART1EN      (1UL << RCC_APB2ENR_USART1EN_Pos)
#define RCC_APB1ENR_USART2EN_Pos  (17U)
#define RCC_APB1ENR_USART2EN      (1UL << RCC_APB1ENR_USART2EN_Pos)
#define RCC_APB2ENR_SYSCFGEN_Pos  (14U)
#define RCC_APB2ENR_SYSCFGEN      (1UL << RCC_APB2ENR_SYSCFGEN_Pos)

/* USART SR register bits */
#define USART_SR_TXE_Pos          (7U)
#define USART_SR_TXE              (1UL << USART_SR_TXE_Pos)
#define USART_SR_RXNE_Pos         (5U)
#define USART_SR_RXNE             (1UL << USART_SR_RXNE_Pos)
#define USART_SR_TC_Pos           (6U)
#define USART_SR_TC               (1UL << USART_SR_TC_Pos)

/* USART CR1 register bits */
#define USART_CR1_UE_Pos          (13U)
#define USART_CR1_UE              (1UL << USART_CR1_UE_Pos)
#define USART_CR1_TE_Pos          (3U)
#define USART_CR1_TE              (1UL << USART_CR1_TE_Pos)
#define USART_CR1_RE_Pos          (2U)
#define USART_CR1_RE              (1UL << USART_CR1_RE_Pos)
#define USART_CR1_RXNEIE_Pos      (5U)
#define USART_CR1_RXNEIE          (1UL << USART_CR1_RXNEIE_Pos)

/* GPIO MODER values */
#define GPIO_MODER_INPUT          (0U)
#define GPIO_MODER_OUTPUT         (1U)
#define GPIO_MODER_AF             (2U)
#define GPIO_MODER_ANALOG         (3U)

/* NVIC functions */
#define NVIC_ISER0                (*(volatile uint32_t *)0xE000E100UL)
#define NVIC_ISER1                (*(volatile uint32_t *)0xE000E104UL)
#define NVIC_ISER2                (*(volatile uint32_t *)0xE000E108UL)

static inline void NVIC_EnableIRQ(uint8_t IRQn) {
    if (IRQn < 32) {
        NVIC_ISER0 = (1UL << IRQn);
    } else if (IRQn < 64) {
        NVIC_ISER1 = (1UL << (IRQn - 32));
    } else {
        NVIC_ISER2 = (1UL << (IRQn - 64));
    }
}

/* IRQ numbers */
#define EXTI0_IRQn                (6)
#define USART1_IRQn               (37)
#define USART2_IRQn               (38)

#endif /* STM32F411XE_H */
