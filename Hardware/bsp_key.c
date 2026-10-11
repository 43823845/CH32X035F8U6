#include "bsp_key.h"

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_APB2PeriphClockCmd(KEY_RCC_PORT, ENABLE);

    GPIO_InitStruct.GPIO_Pin = KEY_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU; /* 上拉输入 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);
}

KeyEvent_t Key_Scan(void)
{
    static uint16_t press_count = 0;
    static uint8_t key_released = 1;
    KeyEvent_t event = KEY_EVENT_NONE;

    /* 按键按下为低电平 */
    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_PIN) == Bit_RESET)
    {
        if (press_count < 1000)
            press_count++;
        
        if (press_count > 60 && key_released) /* 约 600ms 触发长按 */
        {
            event = KEY_EVENT_LONG_PRESS;
            key_released = 0;
        }
    }
    else
    {
        /* 释放检测 */
        if (press_count >= 2 && press_count <= 60 && key_released)
        {
            event = KEY_EVENT_CLICK;
        }
        press_count = 0;
        key_released = 1;
    }

    return event;
}
