#include "bsp_key.h"

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. 初始化 PA2 (SW2 用户按键): 上拉输入 */
    RCC_APB2PeriphClockCmd(KEY_RCC_PORT, ENABLE);

    GPIO_InitStruct.GPIO_Pin = KEY_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU; /* 上拉输入 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);

    /* 2. 初始化 PC17 (SW1 复用按键): 使能 GPIOC 时钟 */
    RCC_APB2PeriphClockCmd(KEY_PC17_RCC, ENABLE);
}

KeyEvent_t Key_Scan(void)
{
    static uint16_t press_count = 0;
    static uint8_t key_released = 1;
    KeyEvent_t event = KEY_EVENT_NONE;

    /* PA2 用户按键按下为低电平 (板载 10k 上拉) */
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

KeyEvent_t Key_PC17_Scan(void)
{
    static uint16_t pc17_press_count = 0;
    static uint8_t  pc17_released    = 1;
    static uint8_t  pc17_idle_level  = 0;
    static uint8_t  pc17_inited      = 0;
    KeyEvent_t event = KEY_EVENT_NONE;

    uint8_t curr = GPIO_ReadInputDataBit(KEY_PC17_PORT, KEY_PC17_PIN);

    /* 首次上电自动捕获当前稳态基准电平 (无论板载为高电平触发还是低电平触发均可完美自适应) */
    if (!pc17_inited)
    {
        pc17_idle_level = curr;
        pc17_inited = 1;
        return KEY_EVENT_NONE;
    }

    if (curr != pc17_idle_level)
    {
        /* 偏离稳态基准，判定为按键被按下 */
        if (pc17_press_count < 1000)
            pc17_press_count++;

        /* 持续超过 1 秒 (100 个 10ms tick) 未恢复，视为外部静态电平变更(如USB插入拔出)，自适应更新基准 */
        if (pc17_press_count > 100)
        {
            pc17_idle_level = curr;
            pc17_press_count = 0;
            pc17_released = 1;
        }
    }
    else
    {
        /* 恢复为稳态基准，判定为按键释放 */
        if (pc17_press_count >= 2 && pc17_press_count <= 60 && pc17_released)
        {
            /* 按下持续 20ms ~ 600ms，识别为有效单击 */
            event = KEY_EVENT_CLICK;
        }
        pc17_press_count = 0;
        pc17_released = 1;
    }

    return event;
}
