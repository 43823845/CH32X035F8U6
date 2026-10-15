#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "ch32x035.h"
#include <stdint.h>

/* SW2 (PA2) 用户按键 (低电平有效) */
#define KEY_PIN         GPIO_Pin_2
#define KEY_GPIO_PORT   GPIOA
#define KEY_RCC_PORT    RCC_APB2Periph_GPIOA

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_CLICK,
    KEY_EVENT_DOUBLE_CLICK,
    KEY_EVENT_LONG_PRESS
} KeyEvent_t;

void Key_Init(void);
KeyEvent_t Key_Scan(void);       /* 扫描 PA2 用户按键 (单击切页/双击调光/长按开关屏) */

#endif /* __BSP_KEY_H */
