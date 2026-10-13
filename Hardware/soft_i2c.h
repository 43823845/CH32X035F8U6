#ifndef __SOFT_I2C_H
#define __SOFT_I2C_H

#include "ch32x035.h"
#include <stdint.h>
#include <stdbool.h>

/* I2C 引脚定义: PB1 - SCL, PB0 - SDA (外部已有 4.7K 上拉电阻) */
#define I2C_GPIO_PORT       GPIOB
#define I2C_RCC_PORT        RCC_APB2Periph_GPIOB
#define I2C_PIN_SCL         GPIO_Pin_1
#define I2C_PIN_SDA         GPIO_Pin_0

/* 基础总线操作 */
void Soft_I2C_Init(void);
bool Soft_I2C_CheckDevice(uint8_t dev_addr);

/* 寄存器抽象读写接口 (通用解耦) */
uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t val);
uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val);
uint8_t I2C_ReadBuffer(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint16_t len);

#endif /* __SOFT_I2C_H */
