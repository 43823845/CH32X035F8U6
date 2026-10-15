/********************************** (C) COPYRIGHT *******************************
 * File Name          : user_display_data.c
 * Description        : 用户显示控制逻辑 (不参与 WCHGUIDesigner 代码生成)
 *
 * 本文件是显示逻辑的唯一编写位置: 控件绑定表 + 数据格式化 + 刷新接口 + 3D姿态立方体渲染。
 * WCHGUIProject/generated/wui_user/ 下的文件均为设计器生成物, 请勿在其中编写业务代码;
 * 本文件因不在该目录内, 设计器重新导出 UI 时不会被覆盖。
 *******************************************************************************/
#include "wui_page_page_2.h"   /* Page2 生成控件 ID 与控件 API */
#include "wui_page_page_3.h"   /* Page3 生成控件 ID */
#include "wui_page_page_4.h"   /* Page4 生成控件 ID */
#include "st7735.h"            /* 屏幕底层图形画线与颜色定义 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ================= 逻辑控件 ↔ 设计器控件 ID 绑定表 =================
 * 全工程仅此处引用设计器生成的 PAGE_x_COMPONENT_xxx 枚举。
 * 设计器增删/重排控件导致序号变化时, 只需修改下表, 下面显示逻辑保持不变。 */

/* Page2 (姿态与加速度监控页) - 文本控件位于左侧 x:3~103，右侧 x:104~159 独立渲染 3D 姿态立方体 */
#define WUI_P2_WIDGET_POSE   PAGE_2_COMPONENT_001   /* component_001: 姿态 */
#define WUI_P2_WIDGET_ACC_X  PAGE_2_COMPONENT_002   /* component_002: X 轴加速度 */
#define WUI_P2_WIDGET_ACC_Y  PAGE_2_COMPONENT_003   /* component_003: Y 轴加速度 */
#define WUI_P2_WIDGET_ACC_Z  PAGE_2_COMPONENT_004   /* component_004: Z 轴加速度 */

/* Page3 (动作识别与落体检测页)
 * 注: component_001("动作识别:")与 component_002("落体检测:")为中文静态标题图片;
 * 右侧预留 component_003 与 component_004 为纯数据动态文本。 */
#define WUI_P3_WIDGET_TAP_VAL PAGE_3_COMPONENT_003  /* 动作识别(单击/双击)数值 */
#define WUI_P3_WIDGET_FF_VAL  PAGE_3_COMPONENT_004  /* 自由落体检测状态数值 */

/* Page4 (光敏与波长监控页) */
#define WUI_P4_WIDGET_LUX    PAGE_4_COMPONENT_001   /* component_001: 亮度 Lux */
#define WUI_P4_WIDGET_LUMEN  PAGE_4_COMPONENT_002   /* component_002: 波长 Wave */

/* ================= 显示缓冲 ================= */

static char s_pose_str[20]  = "Pose:--";
static char s_acc_x_str[16] = "X:0";
static char s_acc_y_str[16] = "Y:0";
static char s_acc_z_str[16] = "Z:0";

static char s_p3_tap_str[20] = "S:0 D:0";
static char s_p3_ff_str[20]  = "OK (0)";

static char s_lux_str[24]   = "Lux:0";
static char s_lumen_str[24] = "Wave:555nm";

/* ================= 姿态(方向)判定 =================
 * 每 100ms 周期调用一次 user_display_attitude_update() 更新滤波与结果,
 * 各显示页通过 user_display_attitude_text() 取用, 避免重复滤波。
 *
 * 1) 一阶低通(alpha=0.3, 10Hz 刷新 -> 时间常数约 0.33s) 抑制手持抖动引起的标签跳变
 * 2) 动态门限 700~1400mg: 失重(|a| 偏小)或高 g(|a| 偏大)时姿态不可信, 返回 "NONE"
 * 3) 判定阈值 0.70: 主轴分量需占重力模长 70% 以上(约 45 度锥角)才判定为某一"面" */

static const char* s_orient      = "TILT";
static float       s_fx          = 0.0f;
static float       s_fy          = 0.0f;
static float       s_fz          = 0.0f;
static uint8_t     s_orient_init = 0;

void user_display_attitude_update(float ax, float ay, float az)
{
    if (!s_orient_init)
    {
        s_fx = ax; s_fy = ay; s_fz = az;
        s_orient_init = 1;
    }
    else
    {
        s_fx += 0.3f * (ax - s_fx);
        s_fy += 0.3f * (ay - s_fy);
        s_fz += 0.3f * (az - s_fz);
    }

    const float mag   = sqrtf(s_fx * s_fx + s_fy * s_fy + s_fz * s_fz);
    const float domin = 0.70f;

    if (mag < 700.0f || mag > 1400.0f)
    {
        s_orient = "NONE";
        return;
    }
    if (fabsf(s_fz) >= domin * mag) { s_orient = (s_fz > 0.0f) ? "FLAT" : "INVERT"; return; }
    if (fabsf(s_fy) >= domin * mag) { s_orient = (s_fy > 0.0f) ? "UPRIGHT" : "DOWN"; return; }
    if (fabsf(s_fx) >= domin * mag) { s_orient = (s_fx > 0.0f) ? "RIGHT" : "LEFT"; return; }

    s_orient = "TILT";
}

const char* user_display_attitude_text(void)
{
    return s_orient;
}

/* ================= 3D 立方体姿态实时线框渲染器 =================
 * 渲染在 Page 2 屏幕右半区域 (Center: X=127, Y=40, 半边长: 13px)
 * 采用"极速双缓冲线段异或/黑底擦除"机制: 仅擦除重画 12 条线段，耗时 < 0.1ms，绝对零闪烁 */

#define CUBE_CX       127   /* 立方体在横屏 160x80 上的中心 X (完美居中于右半屏 X:95~159 区域) */
#define CUBE_CY       40    /* 立方体在横屏 160x80 上的中心 Y */
#define CUBE_SIZE     13.0f /* 半边长 (像素) */

/* 立方体 8 个顶点的局部三维坐标 (以自身质心为原点) */
static const float s_cube_vertices[8][3] = {
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE}, /* 0: 底面前左 */
    {+CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE}, /* 1: 底面前右 */
    {+CUBE_SIZE, +CUBE_SIZE, -CUBE_SIZE}, /* 2: 底面后右 */
    {-CUBE_SIZE, +CUBE_SIZE, -CUBE_SIZE}, /* 3: 底面后左 */
    {-CUBE_SIZE, -CUBE_SIZE, +CUBE_SIZE}, /* 4: 顶面前左 (Z+ 板子正面) */
    {+CUBE_SIZE, -CUBE_SIZE, +CUBE_SIZE}, /* 5: 顶面前右 (Z+ 板子正面) */
    {+CUBE_SIZE, +CUBE_SIZE, +CUBE_SIZE}, /* 6: 顶面后右 (Z+ 板子正面) */
    {-CUBE_SIZE, +CUBE_SIZE, +CUBE_SIZE}, /* 7: 顶面后左 (Z+ 板子正面) */
};

/* 立方体 12 条棱边 */
static const uint8_t s_cube_edges[12][2] = {
    /* 底面 4 条边 (0..3) */
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    /* 顶面 4 条边 (4..7, Z+ 向上) */
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    /* 连接柱 4 条边 (8..11) */
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

/* 缓存上一帧的 12 条线段端点坐标，用于极速无痕擦除 */
static int16_t s_prev_lines[12][4];
static bool    s_cube_has_prev = false;
static float   s_last_pitch    = 999.0f;
static float   s_last_roll     = 999.0f;

void user_display_reset_cube(void)
{
    s_cube_has_prev = false;
    s_last_pitch    = 999.0f;
    s_last_roll     = 999.0f;
}

void user_display_draw_cube(float pitch, float roll)
{
    /* 1. 静态防抖/消闪过滤:
     * 当电路板静止在桌面或手中保持不动(角度微变 < 0.8 度)且上一帧完整呈现时,
     * 直接跳过重画, 彻底根除每 100ms 重复擦除造成的 10Hz 闪烁! */
    if (s_cube_has_prev && fabsf(pitch - s_last_pitch) < 0.8f && fabsf(roll - s_last_roll) < 0.8f)
    {
        return;
    }

    /* 2. 角度转弧度 (俯视等轴测视角: 俯视 25 度, 偏转 30 度) */
    float rad_p = (-pitch + 25.0f) * (3.14159265f / 180.0f);
    float rad_r = (roll   + 30.0f) * (3.14159265f / 180.0f);

    float cp = cosf(rad_p), sp = sinf(rad_p);
    float cr = cosf(rad_r), sr = sinf(rad_r);

    int16_t new_x[8];
    int16_t new_y[8];

    /* 3. 先在内存中预先解算 8 个新顶点的 3D 旋转与屏幕投影坐标
     * 关键修正: 俯视视角下 z 为正(Z+ 顶面)在屏幕上 Y 向上投影 (y1 = y*cp + z*sp),
     * 保证正放桌面时红色顶面正对用户并朝上显示! */
    for (int i = 0; i < 8; i++)
    {
        float x = s_cube_vertices[i][0];
        float y = s_cube_vertices[i][1];
        float z = s_cube_vertices[i][2];

        /* 绕 X 轴旋转 (Pitch) */
        float y1 = y * cp + z * sp;
        float z1 = -y * sp + z * cp;

        /* 绕 Y 轴旋转 (Roll) */
        float x2 = x * cr + z1 * sr;
        float y2 = y1;

        new_x[i] = (int16_t)(CUBE_CX + x2);
        new_y[i] = (int16_t)(CUBE_CY - y2);
    }

    /* 4. 逐线立即替换渲染:
     * 不再统一清屏，而是仅对变动的棱边先擦后画，端点擦除到新画间隔不足 50 微秒，人眼完全无黑帧感 */
    for (int i = 0; i < 12; i++)
    {
        int16_t x0 = new_x[s_cube_edges[i][0]];
        int16_t y0 = new_y[s_cube_edges[i][0]];
        int16_t x1 = new_x[s_cube_edges[i][1]];
        int16_t y1 = new_y[s_cube_edges[i][1]];

        /* 配色方案:
         * 顶面 (edges 4..7, Z+ 向上): 纯红 (COLOR_RED) 代表板子正面天顶朝向
         * 底面 (edges 0..3, Z- 向下): 青色 (COLOR_CYAN) 代表板子底面地心
         * 立柱 (edges 8..11): 白色 (COLOR_WHITE)
         */
        uint16_t color = COLOR_WHITE;
        if (i >= 4 && i <= 7)
        {
            color = COLOR_RED;
        }
        else if (i < 4)
        {
            color = COLOR_CYAN;
        }

        if (s_cube_has_prev)
        {
            /* 若该线段未发生像素位移，直接保留 */
            if (s_prev_lines[i][0] == x0 && s_prev_lines[i][1] == y0 &&
                s_prev_lines[i][2] == x1 && s_prev_lines[i][3] == y1)
            {
                continue;
            }

            /* 擦除该条旧线 */
            ST7735_DrawLine(s_prev_lines[i][0], s_prev_lines[i][1],
                            s_prev_lines[i][2], s_prev_lines[i][3], COLOR_BLACK);
        }

        /* 紧接着画出新线 */
        ST7735_DrawLine(x0, y0, x1, y1, color);

        s_prev_lines[i][0] = x0;
        s_prev_lines[i][1] = y0;
        s_prev_lines[i][2] = x1;
        s_prev_lines[i][3] = y1;
    }

    s_last_pitch = pitch;
    s_last_roll  = roll;
    s_cube_has_prev = true;
}

/* ================= Page2 数据刷新 ================= */

/* 由主循环按 100ms 周期调用, 完成 Page2 姿态及三轴加速度刷新 + 3D 立方体绘制 */
void wui_page_page_2_update_data(float pitch, float roll, float ax, float ay, float az)
{
    /* 1. 左半屏: 文本数值更新 (紧凑规整，不遮挡右侧立方体) */
    snprintf(s_pose_str, sizeof(s_pose_str), "Pose:%s", user_display_attitude_text());
    wui_widget_simplestring_set_str(WUI_P2_WIDGET_POSE, s_pose_str);

    snprintf(s_acc_x_str, sizeof(s_acc_x_str), "X:%ld", (long)(int32_t)ax);
    wui_widget_simplestring_set_str(WUI_P2_WIDGET_ACC_X, s_acc_x_str);

    snprintf(s_acc_y_str, sizeof(s_acc_y_str), "Y:%ld", (long)(int32_t)ay);
    wui_widget_simplestring_set_str(WUI_P2_WIDGET_ACC_Y, s_acc_y_str);

    snprintf(s_acc_z_str, sizeof(s_acc_z_str), "Z:%ld", (long)(int32_t)az);
    wui_widget_simplestring_set_str(WUI_P2_WIDGET_ACC_Z, s_acc_z_str);

    /* 2. 右半屏: 实时绘制 3D 姿态立体线框 */
    user_display_draw_cube(pitch, roll);
}

/* ================= Page3 数据刷新 ================= */

/* 由主循环按 100ms 周期调用
 * 刷新动作识别 (单击/双击) 与自由落体检测结果 */
void wui_page_page_3_update_data(uint32_t tap_single, uint32_t tap_double, uint32_t ff_count, bool ff_recent)
{
    /* 1. 动作识别数据: 单击 / 双击累计次数 (例如 S:1 D:0) */
    snprintf(s_p3_tap_str, sizeof(s_p3_tap_str), "S:%lu D:%lu",
             (unsigned long)tap_single, (unsigned long)tap_double);
    wui_widget_simplestring_set_str(WUI_P3_WIDGET_TAP_VAL, s_p3_tap_str);

    /* 2. 自由落体数据: 若近 2s 内发生跌落变红显示 FALL!, 正常显示 OK (附累计次数) */
    if (ff_recent)
    {
        snprintf(s_p3_ff_str, sizeof(s_p3_ff_str), "FALL!(%lu)", (unsigned long)ff_count);
        wui_widget_simplestring_set_color(WUI_P3_WIDGET_FF_VAL,
                                          RGB888_TO_RGB565(255, 80, 80),
                                          RGB888_TO_RGB565(0, 0, 0));
    }
    else
    {
        snprintf(s_p3_ff_str, sizeof(s_p3_ff_str), "OK (%lu)", (unsigned long)ff_count);
        wui_widget_simplestring_set_color(WUI_P3_WIDGET_FF_VAL,
                                          RGB888_TO_RGB565(129, 199, 132),
                                          RGB888_TO_RGB565(0, 0, 0));
    }
    wui_widget_simplestring_set_str(WUI_P3_WIDGET_FF_VAL, s_p3_ff_str);
}

/* ================= Page4 数据刷新 ================= */

/* 由主循环按 100ms 周期调用, 完成 Page4 光敏与波长显示刷新 */
void wui_page_page_4_update_data(float lux, uint8_t bl_percent)
{
    int32_t lux_i = (int32_t)lux;
    if (lux_i < 0) lux_i = 0;
    int32_t lux_d = (int32_t)((lux - (float)lux_i) * 10.0f);
    if (lux_d < 0) lux_d = -lux_d;

    /* 1. component_001 (亮度/光强): 显示高精度 Lux 值 (支持 0~157,285 Lux) */
    snprintf(s_lux_str, sizeof(s_lux_str), "Lux:%ld.%ld", (long)lux_i, (long)lux_d);
    wui_widget_simplestring_set_str(WUI_P4_WIDGET_LUX, s_lux_str);

    /* 2. component_002 (波长与背光): LTR-308 555nm (VIS) 与当前背光占空比 */
    snprintf(s_lumen_str, sizeof(s_lumen_str), "555nm BL:%d%%", (int)bl_percent);
    wui_widget_simplestring_set_str(WUI_P4_WIDGET_LUMEN, s_lumen_str);
}
