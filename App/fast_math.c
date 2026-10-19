/********************************** (C) COPYRIGHT *******************************
 * File Name          : fast_math.c
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : 高性能轻量级单精度数学库实现
 *******************************************************************************/
#include "fast_math.h"

float fast_atan2f(float y, float x)
{
    float abs_y = fast_fabsf(y);
    float abs_x = fast_fabsf(x);

    if (abs_x < 1e-7f && abs_y < 1e-7f)
    {
        return 0.0f;
    }

    float a, r;
    if (abs_x >= abs_y)
    {
        a = abs_y / abs_x;
    }
    else
    {
        a = abs_x / abs_y;
    }

    /* 7阶极值多项式逼近 atan(a), a in [0, 1] */
    float s = a * a;
    r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * (s * a) + a;

    if (abs_y > abs_x)
    {
        r = FAST_PI_2 - r;
    }
    if (x < 0.0f)
    {
        r = FAST_PI - r;
    }
    if (y < 0.0f)
    {
        r = -r;
    }

    return r;
}

void fast_sincosf(float rad, float *s, float *c)
{
    /* 归一化至 [-PI, PI] */
    while (rad > FAST_PI)  rad -= FAST_2_PI;
    while (rad < -FAST_PI) rad += FAST_2_PI;

    int sign_c = 1;
    if (rad > FAST_PI_2)
    {
        rad = FAST_PI - rad;
        sign_c = -1;
    }
    else if (rad < -FAST_PI_2)
    {
        rad = -FAST_PI - rad;
        sign_c = -1;
    }

    float x2 = rad * rad;

    /* 多项式逼近 sin(x) */
    float sin_val = rad * (1.0f + x2 * (-0.16666657f + x2 * (0.008333075f - 0.000198107f * x2)));

    /* 多项式逼近 cos(x) */
    float cos_val = 1.0f + x2 * (-0.5f + x2 * (0.04166664f - 0.0013888397f * x2));

    if (s) *s = sin_val;
    if (c) *c = (sign_c < 0) ? -cos_val : cos_val;
}

float fast_sqrtf(float x)
{
    if (x <= 0.0f) return 0.0f;

    union {
        float f;
        uint32_t i;
    } u;

    u.f = x;
    u.i = 0x1FBD1DF5 + (u.i >> 1);

    /* 两次牛顿-拉弗森迭代: y = 0.5 * (y + x / y) */
    u.f = 0.5f * (u.f + x / u.f);
    u.f = 0.5f * (u.f + x / u.f);

    return u.f;
}
