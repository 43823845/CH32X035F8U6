#ifndef __BSP_BACKLIGHT_H
#define __BSP_BACKLIGHT_H

#include "ch32x035.h"
#include <stdint.h>

#define BL_PIN          GPIO_Pin_3
#define BL_GPIO_PORT    GPIOA
#define BL_RCC_PORT     RCC_APB2Periph_GPIOA

void Backlight_Init(void);
void Backlight_SetBrightness(uint8_t percent); /* 0 - 100 */
uint8_t Backlight_GetBrightness(void);
uint8_t Backlight_CycleBrightness(void);       /* 3个挡位循环切换 (100% -> 60% -> 20% -> 100%) */
void Backlight_On(void);
void Backlight_Off(void);

#endif /* __BSP_BACKLIGHT_H */
