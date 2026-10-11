#ifndef __LTR308_H
#define __LTR308_H

#include <stdint.h>
#include <stdbool.h>

#define LTR308_I2C_ADDR         0x53

/* 寄存器定义 */
#define LTR308_REG_MAIN_CTRL    0x00
#define LTR308_REG_MEAS_RATE    0x04
#define LTR308_REG_GAIN         0x05
#define LTR308_REG_PART_ID      0x06
#define LTR308_REG_MAIN_STATUS  0x07
#define LTR308_REG_DATA_0       0x0D
#define LTR308_REG_DATA_1       0x0E
#define LTR308_REG_DATA_2       0x0F
#define LTR308_REG_INT_CFG      0x19
#define LTR308_REG_INT_PST      0x1A
#define LTR308_REG_THRES_UP_0   0x21
#define LTR308_REG_THRES_UP_1   0x22
#define LTR308_REG_THRES_UP_2   0x23
#define LTR308_REG_THRES_LOW_0  0x24
#define LTR308_REG_THRES_LOW_1  0x25
#define LTR308_REG_THRES_LOW_2  0x26

/* 增益定义 */
typedef enum {
    LTR308_GAIN_1X  = 0,
    LTR308_GAIN_3X  = 1,
    LTR308_GAIN_6X  = 2,
    LTR308_GAIN_9X  = 3,
    LTR308_GAIN_18X = 4
} LTR308_Gain_t;

bool LTR308_Init(void);
bool LTR308_SetThreshold(uint32_t low_thresh, uint32_t high_thresh);
uint32_t LTR308_ReadRaw(void);
float LTR308_ReadLux(void);

#endif /* __LTR308_H */
