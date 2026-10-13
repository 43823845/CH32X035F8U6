#include "st7735.h"
#include "spi_driver.h"
#include "debug.h"

#define RES_L()     GPIO_ResetBits(ST7735_RES_PORT, ST7735_RES_PIN)
#define RES_H()     GPIO_SetBits(ST7735_RES_PORT, ST7735_RES_PIN)
#define DC_CMD()    GPIO_ResetBits(ST7735_DC_PORT, ST7735_DC_PIN)
#define DC_DATA()   GPIO_SetBits(ST7735_DC_PORT, ST7735_DC_PIN)

#define ST7735_OFFSET_X     1
#define ST7735_OFFSET_Y     26

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

void ST7735_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint16_t rx0 = x0 + ST7735_OFFSET_X;
    uint16_t rx1 = x1 + ST7735_OFFSET_X;
    uint16_t ry0 = y0 + ST7735_OFFSET_Y;
    uint16_t ry1 = y1 + ST7735_OFFSET_Y;

    uint8_t caset[4] = { (uint8_t)(rx0 >> 8), (uint8_t)rx0, (uint8_t)(rx1 >> 8), (uint8_t)rx1 };
    uint8_t raset[4] = { (uint8_t)(ry0 >> 8), (uint8_t)ry0, (uint8_t)(ry1 >> 8), (uint8_t)ry1 };

    /* 列地址设置 (CASET) */
    ST7735_WriteCommand(0x2A);
    DC_DATA();
    SPI1_WriteBuffer(caset, 4);

    /* 行地址设置 (RASET) */
    ST7735_WriteCommand(0x2B);
    DC_DATA();
    SPI1_WriteBuffer(raset, 4);

    /* 写显存命令 (RAMWR) */
    ST7735_WriteCommand(0x2C);
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

    /* 方向设置 (默认横屏 160x80: BGR, MV=1, MX=1) */
    ST7735_WriteCommand(0x36);
    ST7735_WriteData(0x78);

    /* 开启反显 (0.96寸 IPS 屏常规特性) */
    ST7735_WriteCommand(0x21);

    /* 开启显示 */
    ST7735_WriteCommand(0x29);
    Delay_Ms(100);

    ST7735_Clear(COLOR_BLACK);
}

void ST7735_Clear(uint16_t color)
{
    ST7735_SetAddressWindow(0, 0, LCD_SCREEN_WIDTH - 1, LCD_SCREEN_HEIGHT - 1);
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;

    DC_DATA();
    uint32_t total = (uint32_t)LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT;
    while (total--)
    {
        while (!(SPI1->STATR & SPI_I2S_FLAG_TXE));
        SPI1->DATAR = hi;
        while (!(SPI1->STATR & SPI_I2S_FLAG_TXE));
        SPI1->DATAR = lo;
    }
    while (SPI1->STATR & SPI_I2S_FLAG_BSY);
}
