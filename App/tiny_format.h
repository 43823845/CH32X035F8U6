/********************************** (C) COPYRIGHT *******************************
 * File Name          : tiny_format.h
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : 超轻量级嵌入式字符串格式化工具 (替换体积庞大的标准 libc snprintf/printf)
 *******************************************************************************/
#ifndef __TINY_FORMAT_H
#define __TINY_FORMAT_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 超轻量级 snprintf
 * 支持常用嵌入式格式符: %s, %c, %d, %i, %u, %ld, %lu, %x, %X, %%
 * 零动态内存分配，代码体积仅约 150 字节
 */
int tiny_snprintf(char *buf, size_t size, const char *fmt, ...);
int tiny_vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* __TINY_FORMAT_H */
