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

    /* 配置中断阈值: 上阈值设为 0，使每次光强采样完成均超出窗口触发 INT2 中断输出 */
    LTR308_SetThreshold(0, 0);

    /* 启动测量 (ALS_EN = 1) */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_MAIN_CTRL, 0x02);

    return true;
}

bool LTR308_SetThreshold(uint32_t low_thresh, uint32_t high_thresh)
{
    /* 写入上阈值 (20-bit) */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_THRES_UP_0, (uint8_t)(high_thresh & 0xFF));
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_THRES_UP_1, (uint8_t)((high_thresh >> 8) & 0xFF));
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_THRES_UP_2, (uint8_t)((high_thresh >> 16) & 0x0F));

    /* 写入下阈值 (20-bit) */
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_THRES_LOW_0, (uint8_t)(low_thresh & 0xFF));
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_THRES_LOW_1, (uint8_t)((low_thresh >> 8) & 0xFF));
    I2C_WriteReg(LTR308_I2C_ADDR, LTR308_REG_THRES_LOW_2, (uint8_t)((low_thresh >> 16) & 0x0F));

    return true;
}

uint32_t LTR308_ReadRaw(void)
{
    uint8_t status = 0;
    /* 读取主状态寄存器 (自动清除 ALS 中断引脚锁存状态) */
    I2C_ReadReg(LTR308_I2C_ADDR, LTR308_REG_MAIN_STATUS, &status);

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
