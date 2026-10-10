#include "soft_i2c.h"
#include "debug.h"

/* 极简 I2C 延时，约 2~3us */
static inline void I2C_Delay(void)
{
    Delay_Us(2);
}

#define SCL_H()    do { GPIOB->BSHR = GPIO_Pin_1; } while(0)
#define SCL_L()    do { GPIOB->BCR  = GPIO_Pin_1; } while(0)

/* PB0 (SDA) 浮空输入模式 (释放总线，由外部 4.7K 电阻上拉为高) */
#define SDA_H()    do { GPIOB->CFGLR = (GPIOB->CFGLR & ~0x0000000Fu) | 0x00000004u; } while(0)

/* PB0 (SDA) 推挽输出模式低电平 (先清零数据，再配置为输出) */
#define SDA_L()    do { GPIOB->BCR = GPIO_Pin_0; GPIOB->CFGLR = (GPIOB->CFGLR & ~0x0000000Fu) | 0x00000003u; } while(0)

#define SDA_READ() ((GPIOB->INDR & GPIO_Pin_0) ? 1 : 0)

void Soft_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB1 (SCL) 配置为推挽输出 50MHz (CNF=00, MODE=11) */
    GPIOB->CFGLR = (GPIOB->CFGLR & ~0x000000F0u) | 0x00000030u;

    SCL_H();
    SDA_H();
}

static void Soft_I2C_Start(void)
{
    SDA_H();
    SCL_H();
    I2C_Delay();
    SDA_L();
    I2C_Delay();
    SCL_L();
    I2C_Delay();
}

static void Soft_I2C_Stop(void)
{
    SDA_L();
    SCL_L();
    I2C_Delay();
    SCL_H();
    I2C_Delay();
    SDA_H();
    I2C_Delay();
}

static uint8_t Soft_I2C_WaitAck(void)
{
    uint8_t timeout = 0;
    SDA_H(); /* 释放总线为输入 */
    I2C_Delay();
    SCL_H();
    I2C_Delay();

    while (SDA_READ())
    {
        timeout++;
        if (timeout > 200)
        {
            Soft_I2C_Stop();
            return 1; /* NACK or timeout */
        }
    }
    SCL_L();
    I2C_Delay();
    return 0; /* ACK */
}

static void Soft_I2C_SendAck(uint8_t ack)
{
    if (ack)
        SDA_H(); /* NACK: 释放为高 */
    else
        SDA_L(); /* ACK: 拉低 */
    I2C_Delay();
    SCL_H();
    I2C_Delay();
    SCL_L();
    I2C_Delay();
    SDA_H();
}

static void Soft_I2C_SendByte(uint8_t byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (byte & 0x80)
            SDA_H();
        else
            SDA_L();
        byte <<= 1;
        I2C_Delay();
        SCL_H();
        I2C_Delay();
        SCL_L();
        I2C_Delay();
    }
}

static uint8_t Soft_I2C_RecvByte(uint8_t ack)
{
    uint8_t byte = 0;
    SDA_H(); /* 释放总线为输入 */
    for (uint8_t i = 0; i < 8; i++)
    {
        byte <<= 1;
        SCL_H();
        I2C_Delay();
        if (SDA_READ())
            byte |= 0x01;
        SCL_L();
        I2C_Delay();
    }
    Soft_I2C_SendAck(ack);
    return byte;
}

bool Soft_I2C_CheckDevice(uint8_t dev_addr)
{
    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 0);
    uint8_t ack = Soft_I2C_WaitAck();
    Soft_I2C_Stop();
    return (ack == 0);
}

uint8_t I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t val)
{
    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (Soft_I2C_WaitAck()) return 1;

    Soft_I2C_SendByte(reg_addr);
    if (Soft_I2C_WaitAck()) return 2;

    Soft_I2C_SendByte(val);
    if (Soft_I2C_WaitAck()) return 3;

    Soft_I2C_Stop();
    return 0;
}

uint8_t I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint8_t *val)
{
    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (Soft_I2C_WaitAck()) return 1;

    Soft_I2C_SendByte(reg_addr);
    if (Soft_I2C_WaitAck()) return 2;

    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 1);
    if (Soft_I2C_WaitAck()) return 3;

    *val = Soft_I2C_RecvByte(1); /* 读单字节发送 NACK */
    Soft_I2C_Stop();
    return 0;
}

uint8_t I2C_ReadBuffer(uint8_t dev_addr, uint8_t reg_addr, uint8_t *buf, uint16_t len)
{
    if (len == 0 || buf == 0) return 1;

    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (Soft_I2C_WaitAck()) return 2;

    Soft_I2C_SendByte(reg_addr);
    if (Soft_I2C_WaitAck()) return 3;

    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 1);
    if (Soft_I2C_WaitAck()) return 4;

    for (uint16_t i = 0; i < len; i++)
    {
        buf[i] = Soft_I2C_RecvByte((i == len - 1) ? 1 : 0);
    }
    Soft_I2C_Stop();
    return 0;
}

uint8_t I2C_WriteBuffer(uint8_t dev_addr, uint8_t reg_addr, const uint8_t *buf, uint16_t len)
{
    Soft_I2C_Start();
    Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (Soft_I2C_WaitAck()) return 1;

    Soft_I2C_SendByte(reg_addr);
    if (Soft_I2C_WaitAck()) return 2;

    for (uint16_t i = 0; i < len; i++)
    {
        Soft_I2C_SendByte(buf[i]);
        if (Soft_I2C_WaitAck()) return 3;
    }
    Soft_I2C_Stop();
    return 0;
}
