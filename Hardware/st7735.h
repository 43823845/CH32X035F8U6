#ifndef __ST7735_H
#define __ST7735_H

#include "ch32x035.h"
#include <stdint.h>

/* 引脚定义: 
 * PA4: TRES (Reset)
 * PA6: TRS  (RS/DC)
 * PA5: TSCL (SPI1_SCK)
 * PA7: TSDA (SPI1_MOSI)
 * CS 硬件接地
 */
#define ST7735_RES_PIN      GPIO_Pin_4
#define ST7735_RES_PORT     GPIOA
#define ST7735_DC_PIN       GPIO_Pin_6
#define ST7735_DC_PORT      GPIOA

/* 屏幕默认物理尺寸 (0.96寸 80x160) */
#define ST7735_WIDTH        80
#define ST7735_HEIGHT       160

/* 常用 16-bit RGB565 颜色定义 */
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_GRAY          0x8410
#define COLOR_DARKGRAY      0x4208
#define COLOR_NAVY          0x000F
#define COLOR_ORANGE        0xFD20

#define LCD_SCREEN_WIDTH    160
#define LCD_SCREEN_HEIGHT   80

void ST7735_Init(void);
void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ST7735_Clear(uint16_t color);

#endif /* __ST7735_H */
