#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "ch32x035.h"

#define LED_PIN         GPIO_Pin_12
#define LED_GPIO_PORT   GPIOB
#define LED_RCC_PORT    RCC_APB2Periph_GPIOB

void LED_Init(void);
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);

#endif /* __BSP_LED_H */
