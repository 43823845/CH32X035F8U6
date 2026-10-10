#include "ltr308.h"
#include "soft_i2c.h"
#include "debug.h"

static LTR308_Gain_t s_current_gain = LTR308_GAIN_3X;

bool LTR308_Init(void)
{
    uint8_t part_id = 0;

    /* 检查设备是否响应从机地址 */
    if (!Soft_I2C_CheckDevice(LTR308_I2C_ADDR))
    {
        return false;
    }

    /* 软件复位 */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_MAIN_CTRL, 0x10);
    Delay_Ms(15);

    /* 读取 PART_ID 校验 */
    if (I2C_ReadReg(LTR308_I2C_ADDR, LTR308_REG_PART_ID, &part_id) != 0)
    {
        return false;
    }

    /* 设置增益: 3X */
    s_current_gain = LTR308_GAIN_3X;
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_GAIN, (uint8_t)s_current_gain);

    /* 设置测量速率: 100ms 积分时间, 100ms 重复率 (0x22) */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_MEAS_RATE, 0x22);

    /* 使能中断输出: INT_EN=1, INT_POL=0 (Active Low 开漏低有效) -> 0x04 (配合硬件 10k 上拉) */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_INT_CFG, 0x04);
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_INT_PST, 0x00);

    /* 启动测量 (ALS_EN = 1) */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_MAIN_CTRL, 0x02);

    return true;
}

uint32_t LTR308_ReadRaw(void)
{
    uint8_t buf[3] = {0};

    /* 顺序读取 DATA_0, DATA_1, DATA_2 */
    if (I2C_ReadBuffer(LTR308_I2C_ADDR, LTR308_REG_DATA_0, buf, 3) != 0)
    {
        return 0;
    }

    uint32_t raw = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | (((uint32_t)buf[2] & 0x0F) << 16);
    return raw;
}

float LTR308_ReadLux(void)
{
    uint32_t raw = LTR308_ReadRaw();

    /* 转换公式: Lux = (0.6 * raw) / (Gain * (IntegTime / 100ms))
     * 当前配置: Gain = 3X, IntegTime = 100ms
     * 则 Lux = (0.6 * raw) / 3.0 = 0.2 * raw = raw / 5.0
     */
    float lux = (float)raw * 0.2f;
    return lux;
}
