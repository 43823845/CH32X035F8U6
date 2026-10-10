#include "app_sensor.h"
#include "ltr308.h"
#include "lis2dw12.h"
#include "bsp_int.h"
#include <math.h>

#define RAD_TO_DEG (180.0f / 3.14159265f)

static App_SensorData_t s_sensor_data = {0};

void App_Sensor_Init(void)
{
    s_sensor_data.ltr308_online  = LTR308_Init();
    s_sensor_data.lis2dw12_online = LIS2DW12_Init();
}

void App_Sensor_Update(void)
{
    /* 1. 同步 INT1 与 INT2 中断指标 */
    s_sensor_data.int1_count  = Bsp_Int_GetInt1Count();
    s_sensor_data.int2_count  = Bsp_Int_GetInt2Count();
    s_sensor_data.int1_active = Bsp_Int_GetInt1Flag();
    s_sensor_data.int2_active = Bsp_Int_GetInt2Flag();

    /* 2. 更新环境光数据 */
    if (s_sensor_data.ltr308_online)
    {
        s_sensor_data.lux = LTR308_ReadLux();
    }
    else
    {
        /* 尝试重连 */
        s_sensor_data.ltr308_online = LTR308_Init();
    }

    /* 3. 更新加速度与姿态数据 */
    if (s_sensor_data.lis2dw12_online)
    {
        LIS2DW12_Data_t acc;
        if (LIS2DW12_ReadData(&acc))
        {
            s_sensor_data.acc_x = acc.x_mg;
            s_sensor_data.acc_y = acc.y_mg;
            s_sensor_data.acc_z = acc.z_mg;

            /* 计算 Pitch 与 Roll */
            float ax = acc.x_mg;
            float ay = acc.y_mg;
            float az = acc.z_mg;

            s_sensor_data.pitch = atan2f(ax, sqrtf(ay * ay + az * az)) * RAD_TO_DEG;
            s_sensor_data.roll  = atan2f(ay, az) * RAD_TO_DEG;
        }
    }
    else
    {
        /* 尝试重连 */
        s_sensor_data.lis2dw12_online = LIS2DW12_Init();
    }
}

const App_SensorData_t* App_Sensor_GetData(void)
{
    return &s_sensor_data;
}
