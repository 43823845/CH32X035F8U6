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
#define LIS2DW12_REG_STATUS     0x27
#define LIS2DW12_REG_OUT_X_L    0x28

#define LIS2DW12_WHO_AM_I_VAL   0x44

typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    float x_mg;
    float y_mg;
    float z_mg;
} LIS2DW12_Data_t;

bool LIS2DW12_Init(void);
bool LIS2DW12_ReadData(LIS2DW12_Data_t *data);

#endif /* __LIS2DW12_H */
