#include "app_sensor.h"
#include "ltr308.h"
#include "lis2dw12.h"
#include "bsp_int.h"
#include <math.h>

#define RAD_TO_DEG (180.0f / 3.14159265f)

/* 自由落体"检测到"在界面上的保持时间 (单位: 100ms 调用周期 -> 20 即 2s) */
#define FF_HOLD_TICKS  20

static App_SensorData_t s_sensor_data = {0};
static uint8_t          s_ff_hold     = 0;

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

            /* 计算 Pitch (Y轴纵向俯仰) 与 Roll (X轴横向滚转) */
            float ax = acc.x_mg;
            float ay = acc.y_mg;
            float az = acc.z_mg;

            s_sensor_data.pitch = atan2f(ay, az) * RAD_TO_DEG;
            s_sensor_data.roll  = atan2f(ax, az) * RAD_TO_DEG;
        }

        /* 4. 读取内建功能事件: 单击 / 双击 / 自由落体 (读取即清除锁存标志) */
        LIS2DW12_Event_t evt;
        if (LIS2DW12_ReadEvent(&evt))
        {
            if (evt.single_tap)
            {
                s_sensor_data.tap_single_count++;
                s_sensor_data.last_tap_axis = evt.tap_axis;
                s_sensor_data.last_tap_sign = evt.tap_sign;
            }
            if (evt.double_tap)
            {
                s_sensor_data.tap_double_count++;
                s_sensor_data.last_tap_axis = evt.tap_axis;
                s_sensor_data.last_tap_sign = evt.tap_sign;
            }
            if (evt.free_fall)
            {
                s_sensor_data.ff_count++;
                s_ff_hold = FF_HOLD_TICKS;
            }
        }
    }
    else
    {
        /* 尝试重连 */
        s_sensor_data.lis2dw12_online = LIS2DW12_Init();
    }

    /* 5. 自由落体标志保持 2s 后自动复位 */
    if (s_ff_hold > 0)
    {
        s_ff_hold--;
    }
    s_sensor_data.ff_recent = (s_ff_hold > 0);
}

const App_SensorData_t* App_Sensor_GetData(void)
{
    return &s_sensor_data;
}
