#include "spi_driver.h"

void SPI1_Driver_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef  SPI_InitStructure  = {0};

    RCC_APB2PeriphClockCmd(SPI1_RCC_PORT | RCC_APB2Periph_SPI1, ENABLE);

    /* 配置 PA5(SCK), PA7(MOSI) 为复用推挽输出 50MHz */
    GPIO_InitStructure.GPIO_Pin = SPI1_PIN_SCK | SPI1_PIN_MOSI;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SPI1_GPIO_PORT, &GPIO_InitStructure);

    /* SPI1 配置: 主机模式, Mode 0 (CPOL_Low, CPHA_1Edge), 8位, 预分频 2 (高速刷新) */
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}

#define SPI_TIMEOUT_COUNT   ((uint32_t)0x0000FFFF)

void SPI1_WriteByte(uint8_t byte)
{
    uint32_t timeout = SPI_TIMEOUT_COUNT;
    /* 等待发送缓冲区空 (直接寄存器判断，性能极致) */
    while (!(SPI1->STATR & SPI_I2S_FLAG_TXE) && timeout)
    {
        timeout--;
    }
    SPI1->DATAR = byte;

    timeout = SPI_TIMEOUT_COUNT;
    /* 等待发送完成 */
    while ((SPI1->STATR & SPI_I2S_FLAG_BSY) && timeout)
    {
        timeout--;
    }
}

void SPI1_WriteBuffer(const uint8_t *buf, uint16_t len)
{
    if (!buf || len == 0) return;

    for (uint16_t i = 0; i < len; i++)
    {
        uint32_t timeout = SPI_TIMEOUT_COUNT;
        while (!(SPI1->STATR & SPI_I2S_FLAG_TXE) && timeout)
        {
            timeout--;
        }
        SPI1->DATAR = buf[i];
    }

    uint32_t timeout = SPI_TIMEOUT_COUNT;
    while ((SPI1->STATR & SPI_I2S_FLAG_BSY) && timeout)
    {
        timeout--;
    }
}
