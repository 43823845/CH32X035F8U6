#ifndef __LIS2DW12_H
#define __LIS2DW12_H

#include <stdint.h>
#include <stdbool.h>

#define LIS2DW12_I2C_ADDR       0x19

/* 寄存器定义 */
#define LIS2DW12_REG_WHO_AM_I   0x0F
#define LIS2DW12_REG_CTRL1      0x20
#define LIS2DW12_REG_CTRL2      0x21
#define LIS2DW12_REG_CTRL3      0x22
#define LIS2DW12_REG_CTRL4_INT1 0x23
#define LIS2DW12_REG_CTRL6      0x25
#define LIS2DW12_REG_CTRL7      0x3F
#define LIS2DW12_REG_STATUS     0x27
#define LIS2DW12_REG_OUT_X_L    0x28

/* 内建功能寄存器 (单击/双击/自由落体) */
#define LIS2DW12_REG_TAP_THS_X   0x30
#define LIS2DW12_REG_TAP_THS_Y   0x31
#define LIS2DW12_REG_TAP_THS_Z   0x32
#define LIS2DW12_REG_INT_DUR     0x33
#define LIS2DW12_REG_WAKE_UP_THS 0x34
#define LIS2DW12_REG_WAKE_UP_DUR 0x35
#define LIS2DW12_REG_FREE_FALL   0x36
#define LIS2DW12_REG_WAKE_UP_SRC 0x38
#define LIS2DW12_REG_TAP_SRC     0x39

#define LIS2DW12_WHO_AM_I_VAL   0x44

typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    float x_mg;
    float y_mg;
    float z_mg;
} LIS2DW12_Data_t;

/* 内建功能事件 (一次读取即清除传感器锁存标志) */
typedef struct {
    bool    single_tap;   /* 单击 */
    bool    double_tap;   /* 双击 */
    uint8_t tap_axis;     /* 敲击轴: 0=X 1=Y 2=Z */
    bool    tap_sign;     /* 敲击方向符号 */
    bool    free_fall;    /* 自由落体 */
} LIS2DW12_Event_t;

bool LIS2DW12_Init(void);
bool LIS2DW12_ReadData(LIS2DW12_Data_t *data);
bool LIS2DW12_ReadEvent(LIS2DW12_Event_t *evt);

#endif /* __LIS2DW12_H */
