/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_cdc.h
 * Author             : WCH / Antigravity
 * Version            : V1.1.0
 * Description        : CH32X035 USB CDC 虚拟串口驱动头文件 (用于 Type-C 调试输出)
 *******************************************************************************/
#ifndef __USB_CDC_H
#define __USB_CDC_H

#include "ch32x035.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 初始化 USB CDC 外设与引脚，并启动全速 USB 枚举
 */
void USB_CDC_Init(void);

/**
 * @brief 判断 USB 是否已成功枚举并配置完成
 * @return true: 已连接且配置完成; false: 未连接
 */
bool USB_CDC_IsConfigured(void);

/**
 * @brief 发送字节缓冲区到 USB CDC 虚拟串口 (非阻塞，存入环形队列)
 * @param buf 数据指针
 * @param len 数据长度
 * @return 实际写入环形队列的字节数
 */
uint32_t USB_CDC_SendBuffer(const uint8_t *buf, uint32_t len);

/**
 * @brief 周期性轮询/维护 USB CDC 发送队列 (若在中断外使用)
 */
void USB_CDC_Poll(void);

#endif /* __USB_CDC_H */
