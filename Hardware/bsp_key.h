#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "ch32x035.h"
#include <stdint.h>

/* SW2 (PA2) 用户按键 (低电平有效) */
#define KEY_PIN         GPIO_Pin_2
#define KEY_GPIO_PORT   GPIOA
#define KEY_RCC_PORT    RCC_APB2Periph_GPIOA

/* SW1 (PC17) 复用按键 (自适应基准电平) */
#define KEY_PC17_PIN    GPIO_Pin_17
#define KEY_PC17_PORT   GPIOC
#define KEY_PC17_RCC    RCC_APB2Periph_GPIOC

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_CLICK,
    KEY_EVENT_DOUBLE_CLICK,
    KEY_EVENT_LONG_PRESS
} KeyEvent_t;

void Key_Init(void);
KeyEvent_t Key_Scan(void);       /* 扫描 PA2 用户按键 (切页/长按关屏) */
KeyEvent_t Key_PC17_Scan(void);  /* 扫描 PC17 复用按键 (3挡调光) */

#endif /* __BSP_KEY_H */
