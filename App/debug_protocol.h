/********************************** (C) COPYRIGHT *******************************
 * File Name          : debug_protocol.h
 * Author             : WCH / Antigravity
 * Version            : V2.0.0
 * Description        : CH32X035 JustFloat 专用高速二进制浮点协议输出
 *                      专为 Synex / VOFA+ 上位机极速波形与 3D 姿态监控设计
 *******************************************************************************/
#ifndef __DEBUG_PROTOCOL_H
#define __DEBUG_PROTOCOL_H

#include "ch32x035.h"
#include "app_sensor.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief JustFloat 协议帧定义 (52 字节)
 * @note  由 12 个小端 float32 通道值及固定的 0x7F800000 帧尾组成。
 *        Synex 选择 "JustFloat" 协议即可零开销极速解析所有通道与 3D 姿态。
 */
typedef struct __attribute__((packed)) {
    float lux;          /* CH0:  光照强度 (Lux) */
    float pitch;        /* CH1:  俯仰角 (度, -90 ~ +90) */
    float roll;         /* CH2:  横滚角 (度, -180 ~ +180) */
    float acc_x;        /* CH3:  X 轴加速度 (mg) */
    float acc_y;        /* CH4:  Y 轴加速度 (mg) */
    float acc_z;        /* CH5:  Z 轴加速度 (mg) */
    float backlight;    /* CH6:  屏幕背光占空比 (%) */
    float tap_single;   /* CH7:  单击事件累计次数 */
    float tap_double;   /* CH8:  双击事件累计次数 */
    float free_fall;    /* CH9:  自由落体累计次数 */
    float int1_count;   /* CH10: LIS2DW12 INT1 计数 */
    float int2_count;   /* CH11: LTR-308 INT2 计数 */
    uint32_t tail;      /* 帧尾: 固定 0x7F800000 (Little-Endian: 00 00 80 7F) */
} Debug_JustFloatFrame_t;

/**
 * @brief 初始化调试协议模块
 */
void DebugProtocol_Init(void);

/**
 * @brief 周期性打包并发送 JustFloat 传感器监控数据
 * @param sensor_data 传感器结构体指针
 * @param backlight 当前背光占空比 (0-100)
 */
void DebugProtocol_SendFrame(const App_SensorData_t *sensor_data, uint8_t backlight);

#endif /* __DEBUG_PROTOCOL_H */
