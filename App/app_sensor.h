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
} App_SensorData_t;

void App_Sensor_Init(void);
void App_Sensor_Update(void);
const App_SensorData_t* App_Sensor_GetData(void);

#endif /* __APP_SENSOR_H */
