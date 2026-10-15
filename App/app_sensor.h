#ifndef __APP_SENSOR_H
#define __APP_SENSOR_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool  ltr308_online;
    bool  lis2dw12_online;
    float lux;
    float acc_x;    /* 单位: mg */
    float acc_y;
    float acc_z;
    float pitch;    /* 俯仰角 (-90 ~ +90 度) */
    float roll;     /* 横滚角 (-180 ~ +180 度) */
    uint32_t int1_count; /* LIS2DW12 INT1 (PA0) 中断触发累计 (选方案A后为姿态事件次数) */
    uint32_t int2_count; /* LTR-308 INT2 (PA1) 中断触发累计 */
    bool  int1_active;   /* INT1 最新中断标志 */
    bool  int2_active;   /* INT2 最新中断标志 */

    /* LIS2DW12 内建功能事件 (单击/双击/自由落体) */
    uint32_t tap_single_count; /* 单击累计 */
    uint32_t tap_double_count; /* 双击累计 */
    uint8_t  last_tap_axis;    /* 最近敲击轴: 0=X 1=Y 2=Z */
    bool     last_tap_sign;    /* 最近敲击方向符号 */
    uint32_t ff_count;         /* 自由落体累计 */
    bool     ff_recent;        /* 最近 2s 内检测到自由落体 */
} App_SensorData_t;

void App_Sensor_Init(void);
void App_Sensor_Update(void);
const App_SensorData_t* App_Sensor_GetData(void);

#endif /* __APP_SENSOR_H */
 