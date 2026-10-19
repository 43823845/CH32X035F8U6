/********************************** (C) COPYRIGHT *******************************
 * File Name          : bsp_int.c
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : PA0(INT1 - LIS2DW12) 与 PA1(INT2 - LTR308) 外部中断检测驱动实现
 *******************************************************************************/
#include "bsp_int.h"

static volatile uint32_t s_int1_count = 0;
static volatile uint32_t s_int2_count = 0;
static volatile bool     s_int1_flag  = false;
static volatile bool     s_int2_flag  = false;

void Bsp_Int_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    EXTI_InitTypeDef EXTI_InitStruct = {0};
    NVIC_InitTypeDef NVIC_InitStruct = {0};

    /* 1. 开启 GPIOA 与 AFIO 时钟 */
    RCC_APB2PeriphClockCmd(INT1_RCC_PORT | RCC_APB2Periph_AFIO, ENABLE);

    /* 2. PA0(INT1) 与 PA1(INT2) 配置为上拉输入 (硬件板载 10k 上拉电阻 R10/R11) */
    GPIO_InitStruct.GPIO_Pin = INT1_PIN | INT2_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(INT1_PORT, &GPIO_InitStruct);

    /* 3. 引脚映射至 EXTI 外部中断线 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, INT1_PIN_SOURCE);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, INT2_PIN_SOURCE);

    /* 4. 配置 EXTI 中断线 0 和 1: 下降沿触发 (开漏低电平有效) */
    EXTI_ClearITPendingBit(INT1_EXTI_LINE | INT2_EXTI_LINE);
    EXTI_InitStruct.EXTI_Line = INT1_EXTI_LINE | INT2_EXTI_LINE;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStruct);

    /* 5. 配置 NVIC / PFIC 快速中断通道 (EXTI7_0_IRQn) */
    NVIC_InitStruct.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

bool Bsp_Int_GetInt1Flag(void)
{
    __disable_irq();
    bool flag = s_int1_flag;
    s_int1_flag = false;
    __enable_irq();
    return flag;
}

bool Bsp_Int_GetInt2Flag(void)
{
    __disable_irq();
    bool flag = s_int2_flag;
    s_int2_flag = false;
    __enable_irq();
    return flag;
}

uint32_t Bsp_Int_GetInt1Count(void)
{
    /* 32-bit 对齐读取在 RISC-V RV32IM 下天然原子 */
    return s_int1_count;
}

uint32_t Bsp_Int_GetInt2Count(void)
{
    /* 32-bit 对齐读取在 RISC-V RV32IM 下天然原子 */
    return s_int2_count;
}

/* ================== EXTI7_0 快速中断服务例程 ================== */
void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void)
{
    if (EXTI_GetITStatus(INT1_EXTI_LINE) != RESET)
    {
        s_int1_count++;
        s_int1_flag = true;
        EXTI_ClearITPendingBit(INT1_EXTI_LINE);
    }

    if (EXTI_GetITStatus(INT2_EXTI_LINE) != RESET)
    {
        s_int2_count++;
        s_int2_flag = true;
        EXTI_ClearITPendingBit(INT2_EXTI_LINE);
    }
}
