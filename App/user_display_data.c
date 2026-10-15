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

/* ================= 真实 3D 实体姿态感知立方体 (Solid 3D Cube) =================
 * 渲染在 Page 2 屏幕右半区域 (X: 94~159, Y: 0~79, 视口中心: X=127, Y=40, 半边长 14px)
 * 采用专业 3D 实体渲染架构:
 * 1. 背面消隐 (Back-Face Culling): 计算 2D 屏幕投影叉乘, 自动剔除背面, 杜绝透视线框混乱与穿透错觉
 * 2. 实体面纯色平滑填充 (Solid Flat Shading): 顶面/前向/侧面分别采用高对比鲜明配色, 立体感极其强烈
 * 3. 亮白色立体轮廓描边 (High-Contrast Edges): 每个可见面带 1px 晶亮边缘, 棱角硬朗清晰
 * 4. 局部动态包围盒极速擦除 (Bounding Box Wipe): 毫秒级黑底清空旧帧, 绝对零残影、零拖尾、无闪烁 */

#define CUBE_CX          127   /* 3D 视口中心 X */
#define CUBE_CY          40    /* 3D 视口中心 Y */
#define CUBE_HALF_SIZE   14.0f /* 立方体半边长 (像素) */

/* 立方体 8 个顶点的局部三维坐标 (质心为原点) */
static const float s_cube_vertices[8][3] = {
    {-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE}, /* 0: 底面前左 */
    {+CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE}, /* 1: 底面前右 */
    {+CUBE_HALF_SIZE, +CUBE_HALF_SIZE, -CUBE_HALF_SIZE}, /* 2: 底面后右 */
    {-CUBE_HALF_SIZE, +CUBE_HALF_SIZE, -CUBE_HALF_SIZE}, /* 3: 底面后左 */
    {-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, +CUBE_HALF_SIZE}, /* 4: 顶面前左 (Z+ 板卡正面) */
    {+CUBE_HALF_SIZE, -CUBE_HALF_SIZE, +CUBE_HALF_SIZE}, /* 5: 顶面前右 (Z+ 板卡正面) */
    {+CUBE_HALF_SIZE, +CUBE_HALF_SIZE, +CUBE_HALF_SIZE}, /* 6: 顶面后右 (Z+ 板卡正面) */
    {-CUBE_HALF_SIZE, +CUBE_HALF_SIZE, +CUBE_HALF_SIZE}, /* 7: 顶面后左 (Z+ 板卡正面) */
};

/* 立方体 6 个表面的拓扑结构与着色 (按自外向内逆时针顺序定义) */
typedef struct {
    uint8_t  v[4];       /* 4 顶点序号 */
    uint16_t fill_color; /* 面填充色彩 */
} CubeFace_t;

static const CubeFace_t s_cube_faces[6] = {
    /* 0: 顶面 (+Z, 板卡正面, 对应屏幕显示朝向) - 高光天青色 */
    {{4, 5, 6, 7}, RGB888_TO_RGB565(0, 210, 255)},
    /* 1: 前侧面 (-Y, 前向边缘) - 珊瑚暖红/亮橙色 */
    {{0, 1, 5, 4}, RGB888_TO_RGB565(255, 95, 60)},
    /* 2: 右侧面 (+X, 右侧边缘) - 翡翠亮绿色 */
    {{1, 2, 6, 5}, RGB888_TO_RGB565(50, 215, 80)},
    /* 3: 后侧面 (+Y, 后向边缘) - 苍青灰蓝色 */
    {{2, 3, 7, 6}, RGB888_TO_RGB565(50, 130, 170)},
    /* 4: 左侧面 (-X, 左侧边缘) - 晶钻紫晶色 */
    {{3, 0, 4, 7}, RGB888_TO_RGB565(180, 70, 230)},
    /* 5: 底面 (-Z, 板卡背面) - 曜石深靛蓝 */
    {{3, 2, 1, 0}, RGB888_TO_RGB565(35, 55, 90)},
};

/* 上一帧的动态包围盒 */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} CubeBox_t;

static CubeBox_t s_prev_box     = {0, 0, 0, 0};
static bool      s_has_prev_box = false;
static float     s_last_pitch   = 999.0f;
static float     s_last_roll    = 999.0f;

void user_display_reset_cube(void)
{
    s_has_prev_box = false;
    s_last_pitch   = 999.0f;
    s_last_roll    = 999.0f;
    s_prev_box.x   = 0;
    s_prev_box.y   = 0;
    s_prev_box.w   = 0;
    s_prev_box.h   = 0;

    /* 彻底清除右半屏 3D 视口物理区域 (X:94~159, Y:0~79) */
    ST7735_FillRect(94, 0, 66, 80, COLOR_BLACK);
}

/**
 * @brief  极速凸四边形扫描行实体填充算法 (带高对比亮色描边)
 */
static void draw_solid_quad(const int16_t x[4], const int16_t y[4], uint16_t fill_color, uint16_t border_color)
{
    int16_t min_y = y[0], max_y = y[0];
    for (int i = 1; i < 4; i++)
    {
        if (y[i] < min_y) min_y = y[i];
        if (y[i] > max_y) max_y = y[i];
    }

    /* 逐行扫描填充四边形内部 */
    for (int16_t scan_y = min_y; scan_y <= max_y; scan_y++)
    {
        int16_t x_int[4];
        int int_count = 0;

        for (int i = 0; i < 4; i++)
        {
            int next = (i + 1) % 4;
            int16_t y0 = y[i], y1 = y[next];
            int16_t x0 = x[i], x1 = x[next];

            if ((y0 <= scan_y && scan_y < y1) || (y1 <= scan_y && scan_y < y0))
            {
                int32_t x_cross = (int32_t)x0 + (int32_t)(scan_y - y0) * (x1 - x0) / (y1 - y0);
                x_int[int_count++] = (int16_t)x_cross;
            }
            else if (y0 == y1 && scan_y == y0)
            {
                x_int[int_count++] = x0;
                x_int[int_count++] = x1;
            }
        }

        if (int_count >= 2)
        {
            int16_t x_left  = x_int[0];
            int16_t x_right = x_int[0];
            for (int k = 1; k < int_count; k++)
            {
                if (x_int[k] < x_left)  x_left  = x_int[k];
                if (x_int[k] > x_right) x_right = x_int[k];
            }
            if (x_left <= x_right)
            {
                ST7735_FillRect((uint16_t)x_left, (uint16_t)scan_y,
                                (uint16_t)(x_right - x_left + 1), 1, fill_color);
            }
        }
    }

    /* 绘制高对比度轮廓线 */
    for (int i = 0; i < 4; i++)
    {
        int next = (i + 1) % 4;
        ST7735_DrawLine(x[i], y[i], x[next], y[next], border_color);
    }
}

void user_display_draw_cube(float pitch, float roll)
{
    /* 1. 静态防抖: 微小变化 (< 0.7 度) 跳过重画，保持绝对无扰动 */
    if (s_has_prev_box && fabsf(pitch - s_last_pitch) < 0.7f && fabsf(roll - s_last_roll) < 0.7f)
    {
        return;
    }

    /* 2. 角度转弧度 (俯仰角取反使抬头后仰、低头前倾，滚转角右倾右转) */
    float rad_p = -pitch * (3.14159265f / 180.0f);
    float rad_r = roll   * (3.14159265f / 180.0f);

    float cp = cosf(rad_p), sp = sinf(rad_p);
    float cr = cosf(rad_r), sr = sinf(rad_r);

    /* 固定相机视角: 方位角 30°，俯视仰角 25° */
    const float ca = 0.86602540f; /* cos(30°) */
    const float sa = 0.50000000f; /* sin(30°) */
    const float ce = 0.90630779f; /* cos(25°) */
    const float se = 0.42261826f; /* sin(25°) */

    int16_t sx[8];
    int16_t sy[8];
    int16_t min_x = 999, max_x = -999;
    int16_t min_y = 999, max_y = -999;

    /* 3. 内存快速批量解算 8 个顶点的旋转与投影 */
    for (int i = 0; i < 8; i++)
    {
        float x = s_cube_vertices[i][0];
        float y = s_cube_vertices[i][1];
        float z = s_cube_vertices[i][2];

        /* 俯仰 Pitch (绕 X 轴旋转) */
        float y1 = y * cp - z * sp;
        float z1 = y * sp + z * cp;
        float x1 = x;

        /* 滚转 Roll (绕 Y 轴旋转) */
        float x2 = x1 * cr + z1 * sr;
        float z2 = -x1 * sr + z1 * cr;
        float y2 = y1;

        /* 相机坐标系转换 */
        float xc = x2 * ca - y2 * sa;
        float yt = x2 * sa + y2 * ca;
        float yc = yt * se + z2 * ce; /* 相机空间 Y 向上 */

        int16_t px = (int16_t)(CUBE_CX + xc + (xc >= 0 ? 0.5f : -0.5f));
        int16_t py = (int16_t)(CUBE_CY - yc + (yc <= 0 ? 0.5f : -0.5f));

        /* 刚性钳位在右半屏视口内 */
        if (px < 95)  px = 95;
        if (px > 159) px = 159;
        if (py < 0)   py = 0;
        if (py > 79)  py = 79;

        sx[i] = px;
        sy[i] = py;

        if (px < min_x) min_x = px;
        if (px > max_x) max_x = px;
        if (py < min_y) min_y = py;
        if (py > max_y) max_y = py;
    }

    /* 4. 局部黑底精准擦除: 仅擦除上一帧动态包围盒区域 (< 1ms)，彻底消除残影与闪烁 */
    if (s_has_prev_box && s_prev_box.w > 0 && s_prev_box.h > 0)
    {
        ST7735_FillRect(s_prev_box.x, s_prev_box.y, s_prev_box.w, s_prev_box.h, COLOR_BLACK);
    }

    /* 5. 遍历 6 个表面: 2D 叉乘背面消隐 (Back-Face Culling) + 实心着色填充 */
    for (int f = 0; f < 6; f++)
    {
        uint8_t i0 = s_cube_faces[f].v[0];
        uint8_t i1 = s_cube_faces[f].v[1];
        uint8_t i2 = s_cube_faces[f].v[2];
        uint8_t i3 = s_cube_faces[f].v[3];

        int16_t x0 = sx[i0], y0 = sy[i0];
        int16_t x1 = sx[i1], y1 = sy[i1];
        int16_t x2 = sx[i2], y2 = sy[i2];

        /* 屏幕坐标 Y 朝下，叉乘 cross < 0 为正面可见面 */
        int32_t cross = (int32_t)(x1 - x0) * (y2 - y0) - (int32_t)(y1 - y0) * (x2 - x0);
        if (cross >= 0)
        {
            continue; /* 背面自动剔除，绝不重叠混乱！ */
        }

        int16_t qx[4] = {sx[i0], sx[i1], sx[i2], sx[i3]};
        int16_t qy[4] = {sy[i0], sy[i1], sy[i2], sy[i3]};

        /* 实体平滑填充 + 纯白高光外轮廓边线 */
        draw_solid_quad(qx, qy, s_cube_faces[f].fill_color, COLOR_WHITE);
    }

    /* 6. 更新动态绘制包围盒 */
    int16_t bx0 = min_x - 2; if (bx0 < 95)  bx0 = 95;
    int16_t bx1 = max_x + 2; if (bx1 > 159) bx1 = 159;
    int16_t by0 = min_y - 2; if (by0 < 0)   by0 = 0;
    int16_t by1 = max_y + 2; if (by1 > 79)  by1 = 79;

    s_prev_box.x = (uint16_t)bx0;
    s_prev_box.y = (uint16_t)by0;
    s_prev_box.w = (uint16_t)(bx1 - bx0 + 1);
    s_prev_box.h = (uint16_t)(by1 - by0 + 1);

    s_last_pitch   = pitch;
    s_last_roll    = roll;
    s_has_prev_box = true;
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
