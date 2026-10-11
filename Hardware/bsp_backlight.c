#include "bsp_backlight.h"

static uint8_t s_brightness = 100;

void Backlight_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStruct = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = {0};
    TIM_OCInitTypeDef       TIM_OCInitStruct = {0};

    RCC_APB2PeriphClockCmd(BL_RCC_PORT, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* PA3 复用推挽输出 (TIM2_CH4) */
    GPIO_InitStruct.GPIO_Pin = BL_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BL_GPIO_PORT, &GPIO_InitStruct);

    /* 配置 TIM2: 时钟分频使计数时钟为 1MHz, ARR=100 对应 10kHz PWM 频率 */
    uint16_t psc = (SystemCoreClock / 1000000) - 1;
    TIM_TimeBaseInitStruct.TIM_Period = 100 - 1; /* 0~99 对应 100 级 */
    TIM_TimeBaseInitStruct.TIM_Prescaler = psc;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);

    /* 配置 TIM2 通道 4 PWM 模式 1 (计数值 < CCR 时输出有效高电平) */
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_Pulse = 100; /* 默认 100% 占空比 */
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC4Init(TIM2, &TIM_OCInitStruct);

    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    Backlight_SetBrightness(100);
}

void Backlight_SetBrightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    s_brightness = percent;
    TIM_SetCompare4(TIM2, percent);
}

uint8_t Backlight_GetBrightness(void)
{
    return s_brightness;
}

void Backlight_On(void)
{
    Backlight_SetBrightness(s_brightness > 0 ? s_brightness : 80);
}

void Backlight_Off(void)
{
    Backlight_SetBrightness(0);
}
