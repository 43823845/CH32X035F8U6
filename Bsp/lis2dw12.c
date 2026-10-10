#include "lis2dw12.h"
#include "soft_i2c.h"
#include "debug.h"

bool LIS2DW12_Init(void)
{
    uint8_t chip_id = 0;

    /* 检查设备是否在线 */
    if (!Soft_I2C_CheckDevice(LIS2DW12_I2C_ADDR))
    {
        return false;
    }

    /* 校验 WHO_AM_I 寄存器 */
    if (I2C_ReadReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_WHO_AM_I, &chip_id) != 0 || chip_id != LIS2DW12_WHO_AM_I_VAL)
    {
        return false;
    }

    /* CTRL2: 开启 BDU (Block Data Update) 和 IF_ADD_INC (自动地址递增) */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL2, 0x0C);

    /* CTRL3: 开漏输出 (PP_OD=1), 低电平有效 (H_LACTIVE=1) -> 0x0A (配合硬件 10k 上拉) */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL3, 0x0A);

    /* CTRL4_INT1: 使能 INT1 引脚上的数据就绪中断 (DRDY) -> 0x01 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL4_INT1, 0x01);

    /* CTRL6: 满量程 ±2g (FS=00, 高性能模式下灵敏度 0.244 mg/LSB) */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL6, 0x00);

    /* CTRL1: 输出速率 100Hz, 高性能模式 (High-Performance 14-bit) -> 0x50 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL1, 0x50);

    return true;
}

bool LIS2DW12_ReadData(LIS2DW12_Data_t *data)
{
    if (data == 0) return false;

    uint8_t buf[6] = {0};

    /* 连续读取 X, Y, Z 的低高字节 (共 6 字节) */
    if (I2C_ReadBuffer(LIS2DW12_I2C_ADDR, LIS2DW12_REG_OUT_X_L, buf, 6) != 0)
    {
        return false;
    }

    /* 组合 16-bit 有符号数 (14-bit 高性能数据左对齐，因此移位 2 位) */
    int16_t x_raw = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t y_raw = (int16_t)((buf[2 + 1] << 8) | buf[2]);
    int16_t z_raw = (int16_t)((buf[4 + 1] << 8) | buf[4]);

    data->raw_x = x_raw >> 2;
    data->raw_y = y_raw >> 2;
    data->raw_z = z_raw >> 2;

    /* 14-bit 灵敏度: ±2g 时 0.244 mg/LSB */
    data->x_mg = (float)data->raw_x * 0.244f;
    data->y_mg = (float)data->raw_y * 0.244f;
    data->z_mg = (float)data->raw_z * 0.244f;

    return true;
}
