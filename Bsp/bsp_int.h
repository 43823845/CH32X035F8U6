/********************************** (C) COPYRIGHT *******************************
 * File Name          : bsp_int.h
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : PA0(INT1 - LIS2DW12) 与 PA1(INT2 - LTR308) 外部中断检测驱动
 *******************************************************************************/
#ifndef __BSP_INT_H
#define __BSP_INT_H

#include "ch32x035.h"
#include <stdint.h>
#include <stdbool.h>

/* PA0: LIS2DW12 INT1 (外部中断线 0) */
#define INT1_PIN            GPIO_Pin_0
#define INT1_PORT           GPIOA
#define INT1_RCC_PORT       RCC_APB2Periph_GPIOA
#define INT1_EXTI_LINE      EXTI_Line0
#define INT1_PIN_SOURCE     GPIO_PinSource0

/* PA1: LTR-308 INT2 (外部中断线 1) */
#define INT2_PIN            GPIO_Pin_1
#define INT2_PORT           GPIOA
#define INT2_RCC_PORT       RCC_APB2Periph_GPIOA
#define INT2_EXTI_LINE      EXTI_Line1
#define INT2_PIN_SOURCE     GPIO_PinSource1

/**
 * @brief 初始化 PA0 (INT1) 和 PA1 (INT2) 外部中断与 NVIC/PFIC
 */
void Bsp_Int_Init(void);

/**
 * @brief 获取并清除 INT1 (LIS2DW12) 触发标志
 * @return true 表示发生过中断
 */
bool Bsp_Int_GetInt1Flag(void);

/**
 * @brief 获取并清除 INT2 (LTR308) 触发标志
 * @return true 表示发生过中断
 */
bool Bsp_Int_GetInt2Flag(void);

/**
 * @brief 获取 INT1 中断触发累计次数
 */
uint32_t Bsp_Int_GetInt1Count(void);

/**
 * @brief 获取 INT2 中断触发累计次数
 */
uint32_t Bsp_Int_GetInt2Count(void);

#endif /* __BSP_INT_H */
