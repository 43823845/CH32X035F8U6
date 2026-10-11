#include "st7735.h"
#include "spi_driver.h"
#include "debug.h"

#define RES_L()     GPIO_ResetBits(ST7735_RES_PORT, ST7735_RES_PIN)
#define RES_H()     GPIO_SetBits(ST7735_RES_PORT, ST7735_RES_PIN)
#define DC_CMD()    GPIO_ResetBits(ST7735_DC_PORT, ST7735_DC_PIN)
#define DC_DATA()   GPIO_SetBits(ST7735_DC_PORT, ST7735_DC_PIN)

static uint16_t s_width  = ST7735_WIDTH;
static uint16_t s_height = ST7735_HEIGHT;
static uint8_t  s_x_offset = 26;
static uint8_t  s_y_offset = 1;

static void ST7735_WriteCommand(uint8_t cmd)
{
    DC_CMD();
    SPI1_WriteByte(cmd);
}

static void ST7735_WriteData(uint8_t data)
{
    DC_DATA();
    SPI1_WriteByte(data);
}

static void ST7735_WriteData16(uint16_t data)
{
    DC_DATA();
    SPI1_WriteByte((uint8_t)(data >> 8));
    SPI1_WriteByte((uint8_t)data);
}

void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint16_t real_x0 = x0 + s_x_offset;
    uint16_t real_x1 = x1 + s_x_offset;
    uint16_t real_y0 = y0 + s_y_offset;
    uint16_t real_y1 = y1 + s_y_offset;

    /* 列地址设置 (CASET) */
    ST7735_WriteCommand(0x2A);
    ST7735_WriteData((uint8_t)(real_x0 >> 8));
    ST7735_WriteData((uint8_t)real_x0);
    ST7735_WriteData((uint8_t)(real_x1 >> 8));
    ST7735_WriteData((uint8_t)real_x1);

    /* 行地址设置 (RASET) */
    ST7735_WriteCommand(0x2B);
    ST7735_WriteData((uint8_t)(real_y0 >> 8));
    ST7735_WriteData((uint8_t)real_y0);
    ST7735_WriteData((uint8_t)(real_y1 >> 8));
    ST7735_WriteData((uint8_t)real_y1);

    /* 写显存命令 (RAMWR) */
    ST7735_WriteCommand(0x2C);
}

void ST7735_SetRotation(uint8_t m)
{
    ST7735_WriteCommand(0x36); /* MADCTL */
    switch (m % 4)
    {
        case 0: /* 竖屏 80x160 */
            ST7735_WriteData(0xC8); /* BGR, MY=1, MX=1 */
            s_width    = 80;
            s_height   = 160;
            s_x_offset = 26;
            s_y_offset = 1;
            break;
        case 1: /* 横屏 160x80 */
            ST7735_WriteData(0x78); /* BGR, MV=1, MX=1 */
            s_width    = 160;
            s_height   = 80;
            s_x_offset = 1;
            s_y_offset = 26;
            break;
        case 2: /* 倒立竖屏 80x160 */
            ST7735_WriteData(0x08); /* BGR */
            s_width    = 80;
            s_height   = 160;
            s_x_offset = 26;
            s_y_offset = 1;
            break;
        case 3: /* 倒立横屏 160x80 */
            ST7735_WriteData(0xA8); /* BGR, MV=1, MY=1 */
            s_width    = 160;
            s_height   = 80;
            s_x_offset = 1;
            s_y_offset = 26;
            break;
    }
}

void ST7735_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* 配置 PA4(Reset), PA6(DC) 为推挽输出 */
    GPIO_InitStruct.GPIO_Pin = ST7735_RES_PIN | ST7735_DC_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 硬件 SPI1 初始化 */
    SPI1_Driver_Init();

    /* 硬件复位 */
    RES_L();
    Delay_Ms(30);
    RES_H();
    Delay_Ms(30);

    /* 退出休眠 */
    ST7735_WriteCommand(0x11);
    Delay_Ms(120);

    /* 帧率控制 (Normal mode) */
    ST7735_WriteCommand(0xB1);
    ST7735_WriteData(0x05);
    ST7735_WriteData(0x3C);
    ST7735_WriteData(0x3C);

    ST7735_WriteCommand(0xB2);
    ST7735_WriteData(0x05);
    ST7735_WriteData(0x3C);
    ST7735_WriteData(0x3C);

    ST7735_WriteCommand(0xB3);
    ST7735_WriteData(0x05);
    ST7735_WriteData(0x3C);
    ST7735_WriteData(0x3C);
    ST7735_WriteData(0x05);
    ST7735_WriteData(0x3C);
    ST7735_WriteData(0x3C);

    /* 点反转控制 */
    ST7735_WriteCommand(0xB4);
    ST7735_WriteData(0x03);

    /* 电源控制 */
    ST7735_WriteCommand(0xC0);
    ST7735_WriteData(0x28);
    ST7735_WriteData(0x08);
    ST7735_WriteData(0x04);

    ST7735_WriteCommand(0xC1);
    ST7735_WriteData(0xC0);

    ST7735_WriteCommand(0xC2);
    ST7735_WriteData(0x0D);
    ST7735_WriteData(0x00);

    ST7735_WriteCommand(0xC3);
    ST7735_WriteData(0x8D);
    ST7735_WriteData(0x2A);

    ST7735_WriteCommand(0xC4);
    ST7735_WriteData(0x8D);
    ST7735_WriteData(0xEE);

    /* VCOM 控制 */
    ST7735_WriteCommand(0xC5);
    ST7735_WriteData(0x1A);

    /* 颜色模式: 16-bit RGB565 */
    ST7735_WriteCommand(0x3A);
    ST7735_WriteData(0x05);

    /* 方向设置 (默认横屏 160x80 便于仪表盘显示) */
    ST7735_SetRotation(1);

    /* 开启反显 (0.96寸 IPS 屏常规特性) */
    ST7735_WriteCommand(0x21);

    /* 开启显示 */
    ST7735_WriteCommand(0x29);
    Delay_Ms(100);

    ST7735_Clear(COLOR_BLACK);
}

void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= s_width || y >= s_height) return;
    ST7735_SetAddressWindow(x, y, x, y);
    ST7735_WriteData16(color);
}

void ST7735_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= s_width || y >= s_height || w == 0 || h == 0) return;
    if (x + w > s_width)  w = s_width - x;
    if (y + h > s_height) h = s_height - y;

    ST7735_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;

    DC_DATA();
    uint32_t total_pixels = (uint32_t)w * h;
    for (uint32_t i = 0; i < total_pixels; i++)
    {
        SPI1_WriteByte(hi);
        SPI1_WriteByte(lo);
    }
}

void ST7735_Clear(uint16_t color)
{
    ST7735_FillRect(0, 0, s_width, s_height, color);
}

uint16_t ST7735_GetWidth(void)
{
    return s_width;
}

uint16_t ST7735_GetHeight(void)
{
    return s_height;
}
