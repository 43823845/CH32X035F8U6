/********************************** (C) COPYRIGHT *******************************
 * File Name          : debug_protocol.c
 * Author             : WCH / Antigravity
 * Version            : V2.0.0
 * Description        : CH32X035 JustFloat 专用高速二进制浮点协议输出实现
 *                      极致轻量，零 CPU 格式化转换开销，纯内存打包直接 DMA/FIFO 发送
 *******************************************************************************/
#include "debug_protocol.h"
#include "usb_cdc.h"

void DebugProtocol_Init(void)
{
    /* 模块初始化 */
}

void DebugProtocol_SendFrame(const App_SensorData_t *sensor_data, uint8_t backlight)
{
    if (!sensor_data) return;

    Debug_JustFloatFrame_t frame;
    frame.lux         = sensor_data->lux;
    frame.pitch       = sensor_data->pitch;
    frame.roll        = sensor_data->roll;
    frame.acc_x       = sensor_data->acc_x;
    frame.acc_y       = sensor_data->acc_y;
    frame.acc_z       = sensor_data->acc_z;
    frame.backlight   = (float)backlight;
    frame.tap_single  = (float)sensor_data->tap_single_count;
    frame.tap_double  = (float)sensor_data->tap_double_count;
    frame.free_fall   = (float)sensor_data->ff_count;
    frame.int1_count  = (float)sensor_data->int1_count;
    frame.int2_count  = (float)sensor_data->int2_count;
    frame.tail        = 0x7F800000u; /* IEEE 754 +Inf (Little-Endian: 00 00 80 7F) */

    USB_CDC_SendBuffer((const uint8_t *)&frame, sizeof(frame));
}
