#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "ch32x035.h"
#include <stdint.h>

#define KEY_PIN         GPIO_Pin_2
#define KEY_GPIO_PORT   GPIOA
#define KEY_RCC_PORT    RCC_APB2Periph_GPIOA

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_CLICK,
    KEY_EVENT_LONG_PRESS
} KeyEvent_t;

void Key_Init(void);
KeyEvent_t Key_Scan(void);

#endif /* __BSP_KEY_H */
