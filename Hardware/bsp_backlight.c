#include "bsp_backlight.h"

static uint8_t s_brightness      = 100;
static uint8_t s_last_brightness = 100;

void Backlight_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = {0};
    TIM_OCInitTypeDef       TIM_OCInitStruct = {0};

    RCC_APB2PeriphClockCmd(BL_RCC_PORT | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

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
    TIM_CtrlPWMOutputs(TIM2, ENABLE); /* 使能主输出 MOE (CH32X035 关键位) */
    TIM_Cmd(TIM2, ENABLE);

    /* 默认开机满亮度点亮背光 */
    Backlight_SetBrightness(100);
}

void Backlight_SetBrightness(uint8_t percent)
{
    if (percent > 100) percent = 100;
    if (percent > 0)
    {
        s_last_brightness = percent;
    }
    s_brightness = percent;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = BL_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

    if (percent >= 100)
    {
        /* 100% 满亮度: 直接配置 PA3 为普通推挽高电平，保证 3.3V 充分导通 DTC143ZE 三极管 */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(BL_GPIO_PORT, &GPIO_InitStruct);
        GPIO_SetBits(BL_GPIO_PORT, BL_PIN);
    }
    else if (percent == 0)
    {
        /* 0% 亮度: 推挽低电平完全关断背光 */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_Init(BL_GPIO_PORT, &GPIO_InitStruct);
        GPIO_ResetBits(BL_GPIO_PORT, BL_PIN);
    }
    else
    {
        /* 1%~99% 亮度: 切换为 TIM2_CH4 复用推挽输出 PWM 调光 */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(BL_GPIO_PORT, &GPIO_InitStruct);

        TIM_SetCompare4(TIM2, percent);
        TIM_CtrlPWMOutputs(TIM2, ENABLE);
    }
}

uint8_t Backlight_GetBrightness(void)
{
    return s_brightness;
}

void Backlight_On(void)
{
    Backlight_SetBrightness(s_last_brightness > 0 ? s_last_brightness : 100);
}

void Backlight_Off(void)
{
    Backlight_SetBrightness(0);
}

/* 3 个循环亮度挡位: 100% (高亮), 60% (中亮), 20% (暗光) */
static const uint8_t s_brightness_levels[3] = {100, 60, 20};
static uint8_t       s_level_index = 0;

uint8_t Backlight_CycleBrightness(void)
{
    if (s_brightness == 0)
    {
        /* 若当前处于关闭状态，直接恢复当前挡位亮度点亮 */
        Backlight_SetBrightness(s_brightness_levels[s_level_index]);
    }
    else
    {
        /* 循环切换下一挡位: 100% -> 60% -> 20% -> 100% */
        s_level_index = (s_level_index + 1) % 3;
        Backlight_SetBrightness(s_brightness_levels[s_level_index]);
    }
    return s_brightness;
}
