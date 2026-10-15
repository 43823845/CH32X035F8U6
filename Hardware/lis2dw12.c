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

    /* CTRL3: 开漏输出 (Bit 5: PP_OD=1), 低电平有效 (Bit 3: H_LACTIVE=1), 中断锁存 (Bit 4: LIR=1) -> 0x38
     * LIR=1 使事件标志保持到源寄存器被读取, 避免 100ms 轮询漏掉毫秒级的敲击脉冲
     * 位定义依据 ST lis2dw12_reg.h: slp_mode[1:0], bit2 未用, H_LACTIVE=bit3, LIR=bit4, PP_OD=bit5 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL3, 0x38);

    /* CTRL4_INT1: INT1 推送
     * 单击(Bit6: INT1_SINGLE_TAP) + 双击(Bit3: INT1_TAP) + 自由落体(Bit4: INT1_FF) -> 0x58 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL4_INT1, 0x58);

    /* 敲击阈值: 5 位, 步进 62.5mg (±2gFS) -> 0x0C 约 750mg; TAP_THS_Z 同时使能 X,Y,Z 三轴 (Bit7/6/5 = 111b) */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_TAP_THS_X, 0x0C);
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_TAP_THS_Y, 0x0C);
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_TAP_THS_Z, 0xE0 | 0x0C);

    /* 敲击时序: ST 推荐配置 Latency=7, Quiet=3, Shock=3 -> 0x7F */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_INT_DUR, 0x7F);

    /* 同时使能单击与双击识别 (Bit7: single_double_tap=1) */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_WAKE_UP_THS, 0x80);

    /* WAKE_UP_DUR: 自由落体时长高位 ff_dur = 0 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_WAKE_UP_DUR, 0x00);

    /* 自由落体: 阈值 ff_ths=011b (约 344mg), 时长 ff_dur=6 (60ms @ 100Hz) -> 0x33 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_FREE_FALL, 0x33);

    /* CTRL7: 开启内建嵌入式功能中断总使能 (Bit 5: INTERRUPTS_ENABLE = 1) -> 0x20
     * 关键配置: 此位为 0 时所有敲击与自由落体引擎被芯片全局旁路关闭，置 1 后引擎开始执行 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL7, 0x20);

    /* CTRL6: 满量程 ±2g (FS=00, 高性能模式下灵敏度 0.244 mg/LSB) */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL6, 0x00);

    /* CTRL1: 输出速率 100Hz (ODR=0101b), 真正高性能模式 (MODE=01b -> 14-bit) -> 0x54 */
    I2C_WriteReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_CTRL1, 0x54);

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

    /* 组合 16-bit 补码数 (14-bit 高性能模式数据左对齐在 bits [15:2]) */
    int16_t x_raw = (int16_t)(((uint16_t)buf[1] << 8) | (uint16_t)buf[0]);
    int16_t y_raw = (int16_t)(((uint16_t)buf[3] << 8) | (uint16_t)buf[2]);
    int16_t z_raw = (int16_t)(((uint16_t)buf[5] << 8) | (uint16_t)buf[4]);

    /* 标准有符号算术缩放为 14-bit 整数 (消除编译器实现定义行为差异) */
    data->raw_x = x_raw / 4;
    data->raw_y = y_raw / 4;
    data->raw_z = z_raw / 4;

    /* 14-bit 灵敏度: ±2g 时 0.244 mg/LSB */
    data->x_mg = (float)data->raw_x * 0.244f;
    data->y_mg = (float)data->raw_y * 0.244f;
    data->z_mg = (float)data->raw_z * 0.244f;

    return true;
}

bool LIS2DW12_ReadEvent(LIS2DW12_Event_t *evt)
{
    if (evt == 0) return false;

    uint8_t tap_src = 0;
    uint8_t wu_src  = 0;

    /* 读取源寄存器即释放 LIR 锁存 (下一次事件可再次触发) */
    if (I2C_ReadReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_TAP_SRC, &tap_src) != 0)
    {
        return false;
    }
    if (I2C_ReadReg(LIS2DW12_I2C_ADDR, LIS2DW12_REG_WAKE_UP_SRC, &wu_src) != 0)
    {
        return false;
    }

    /* TAP_SRC (ST lis2dw12_reg.h 位定义):
     * bit0/1/2 = z/y/x_tap, bit3 = tap_sign, bit4 = double_tap, bit5 = single_tap, bit6 = tap_ia */
    evt->single_tap = (tap_src & 0x20) != 0;   /* bit5: single_tap */
    evt->double_tap = (tap_src & 0x10) != 0;   /* bit4: double_tap */
    evt->tap_sign   = (tap_src & 0x08) != 0;
    evt->tap_axis   = (tap_src & 0x04) ? 0 : ((tap_src & 0x02) ? 1 : 2);

    /* WAKE_UP_SRC: bit5=ff_ia (自由落体) */
    evt->free_fall  = (wu_src & 0x20) != 0;

    return true;
}
