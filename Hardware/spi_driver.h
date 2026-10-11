#ifndef __SPI_DRIVER_H
#define __SPI_DRIVER_H

#include "ch32x035.h"
#include <stdint.h>

/* SPI1 引脚配置: PA5 - SCK, PA7 - MOSI (屏幕只写模式) */
#define SPI1_GPIO_PORT      GPIOA
#define SPI1_RCC_PORT       RCC_APB2Periph_GPIOA
#define SPI1_PIN_SCK        GPIO_Pin_5
#define SPI1_PIN_MOSI       GPIO_Pin_7

void SPI1_Driver_Init(void);
void SPI1_WriteByte(uint8_t byte);
void SPI1_WriteBuffer(const uint8_t *buf, uint16_t len);

#endif /* __SPI_DRIVER_H */
