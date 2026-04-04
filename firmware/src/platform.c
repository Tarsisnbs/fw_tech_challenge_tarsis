#include "platform.h"
#include "stm32f411xe.h"
#include "FreeRTOS.h"

/*

*/

void platform_init(void){
    volatile uint8_t *nvic_ipr = (volatile uint8_t *)0xE000E400;
    nvic_ipr[38] = (uint8_t)(7 << 4); // Define priority 7 (Safe for FreeRTOS)
    
}