#include "bsp_led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_APB2PeriphClockCmd(LED_RCC_PORT, ENABLE);

    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    LED_Off(); /* 默认关闭 (输出高电平) */
}

void LED_On(void)
{
    GPIO_ResetBits(LED_GPIO_PORT, LED_PIN); /* 低电平点亮 */
}

void LED_Off(void)
{
    GPIO_SetBits(LED_GPIO_PORT, LED_PIN);   /* 高电平熄灭 */
}

void LED_Toggle(void)
{
    if (GPIO_ReadOutputDataBit(LED_GPIO_PORT, LED_PIN))
        GPIO_ResetBits(LED_GPIO_PORT, LED_PIN);
    else
        GPIO_SetBits(LED_GPIO_PORT, LED_PIN);
}
