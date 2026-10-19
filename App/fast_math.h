/********************************** (C) COPYRIGHT *******************************
 * File Name          : fast_math.h
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : 高性能轻量级单精度数学库 (替换庞大且引入双精度除法的标准 libm.a)
 *******************************************************************************/
#ifndef __FAST_MATH_H
#define __FAST_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FAST_PI         3.14159265f
#define FAST_PI_2       1.57079632f
#define FAST_2_PI       6.28318530f
#define FAST_DEG_TO_RAD (FAST_PI / 180.0f)
#define FAST_RAD_TO_DEG (180.0f / FAST_PI)

/**
 * @brief 单精度绝对值 (极速无分支)
 */
static inline float fast_fabsf(float x)
{
    return (x < 0.0f) ? -x : x;
}

/**
 * @brief 单精度反正切 atan2f(y, x) 极速高精度逼近
 * @return 弧度 [-pi, pi]，最大角度误差 < 0.04°，完全胜任 IMU 姿态解算
 */
float fast_atan2f(float y, float x);

/**
 * @brief 单精度正弦与余弦同时快速解算
 * @param rad 输入弧度
 * @param s 输出正弦值
 * @param c 输出余弦值
 */
void fast_sincosf(float rad, float *s, float *c);

static inline float fast_sinf(float rad)
{
    float s, c;
    fast_sincosf(rad, &s, &c);
    return s;
}

static inline float fast_cosf(float rad)
{
    float s, c;
    fast_sincosf(rad, &s, &c);
    return c;
}

/**
 * @brief 单精度快速开方 sqrtf(x)
 * @param x 非负浮点数
 * @return 算术平方根，相对误差 < 1e-6
 */
float fast_sqrtf(float x);

#ifdef __cplusplus
}
#endif

#endif /* __FAST_MATH_H */
