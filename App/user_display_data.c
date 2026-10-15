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

/* ================= 航空姿态仪 / 仪表级水平地平仪 (Attitude Indicator) =================
 * 渲染在 Page 2 屏幕右半区域 (X: 94~159, Y: 0~79, 仪表盘中心: X=127, Y=40, 半径 R=26)
 * 采用专业航空仪表 PFD (Primary Flight Display) 标准视觉设计:
 * 1. 动态双色天地分界: 经典航空深蓝天空 (Sky) + 暖棕大地 (Ground) + 纯白人工地平线 (Horizon)
 * 2. 逐行全覆写极速扫描渲染 (Scanline Direct Fill): 纯硬件级刷新, 无重影、无残影、零闪烁
 * 3. 动态俯仰梯形标尺刻度 (Pitch Ladder): 随滚转与俯仰精准旋转与平移 (±10°, ±20°)
 * 4. 中央固定飞机基准符号 (Fixed Aircraft Symbol): 经典亮黄双翼与瞄准中轴标
 * 5. 外圈仪表环与滚转刻度 (Roll Scale): 顶部 12 点钟指引白三角标与 ±30°/±60° 刻度线 */

#define HORIZON_CX       127   /* 仪表盘中心 X */
#define HORIZON_CY       40    /* 仪表盘中心 Y */
#define HORIZON_R        26    /* 仪表盘半径 (外径 53 像素, 严密受限于 94~159 区域) */
#define PITCH_SCALE      0.55f /* 俯仰角像素比例 (像素/度) */

/* 航空仪表专用色彩定义 (16-bit RGB565) */
#define COLOR_AV_SKY       RGB888_TO_RGB565(25, 95, 190)   /* 经典航空天蓝 */
#define COLOR_AV_GROUND    RGB888_TO_RGB565(130, 65, 18)   /* 经典航空大地暖棕 */
#define COLOR_AV_BEZEL     RGB888_TO_RGB565(160, 172, 190) /* 外圈仪表刻度环钛银灰 */
#define COLOR_AV_AIRCRAFT  RGB888_TO_RGB565(255, 215, 0)   /* 中央固定战机符号 (亮金黄) */

static bool  s_has_prev_attitude = false;
static float s_last_pitch        = 999.0f;
static float s_last_roll         = 999.0f;

void user_display_reset_cube(void)
{
    s_has_prev_attitude = false;
    s_last_pitch        = 999.0f;
    s_last_roll         = 999.0f;

    /* 彻底清除右半屏姿态仪视口物理区域 (X:94~159, Y:0~79) */
    ST7735_FillRect(94, 0, 66, 80, COLOR_BLACK);
}

void user_display_draw_cube(float pitch, float roll)
{
    /* 1. 静态防抖: 微小变化 (< 0.6 度) 且上一帧已完整渲染时跳过刷新，杜绝无意义重绘 */
    if (s_has_prev_attitude && fabsf(pitch - s_last_pitch) < 0.6f && fabsf(roll - s_last_roll) < 0.6f)
    {
        return;
    }

    /* 限制俯仰显示范围，防止地平线完全移出仪表圈 */
    float pitch_clamped = pitch;
    if (pitch_clamped > 38.0f)  pitch_clamped = 38.0f;
    if (pitch_clamped < -38.0f) pitch_clamped = -38.0f;

    /* 角度转弧度 (Roll: 右滚转为正，地平线相对逆时针转) */
    float rad = roll * (3.14159265f / 180.0f);
    float sin_r = sinf(rad);
    float cos_r = cosf(rad);

    /* 俯仰引起的地平线中心在屏幕 Y 轴上的位移 (抬头发正: 地平线向下移; 低头发负: 地平线向上移) */
    float dy_h = pitch_clamped * PITCH_SCALE;

    /* 2. 逐行全覆写扫描式渲染圆形仪表盘 (内部像素单帧 100% 覆盖，绝无残影与拖尾) */
    uint16_t row_buf[56];
    const int16_t R = HORIZON_R;
    const int32_t R_sq = R * R;
    const int32_t R_inner_sq = (R - 1) * (R - 1);

    for (int16_t dy = -R; dy <= R; dy++)
    {
        int32_t dy2 = (int32_t)dy * dy;
        if (dy2 > R_sq) continue;

        int16_t dx_max = (int16_t)sqrtf((float)(R_sq - dy2));
        int16_t x0 = HORIZON_CX - dx_max;
        int16_t x1 = HORIZON_CX + dx_max;
        uint16_t len = (uint16_t)(x1 - x0 + 1);
        if (len > 55) len = 55;

        float c_y = (dy - dy_h) * cos_r;
        float h = (float)(dx_max) * sin_r - c_y;
        float dh = -sin_r;

        for (uint16_t i = 0; i < len; i++)
        {
            int16_t dx = -dx_max + (int16_t)i;
            int32_t r2 = (int32_t)dx * dx + dy2;

            if (r2 >= R_inner_sq)
            {
                row_buf[i] = COLOR_AV_BEZEL; /* 刻度盘边缘环 */
            }
            else
            {
                if (fabsf(h) <= 0.85f)
                {
                    row_buf[i] = COLOR_WHITE;       /* 白色人工地平线 */
                }
                else if (h > 0.0f)
                {
                    row_buf[i] = COLOR_AV_SKY;      /* 天空深蓝 */
                }
                else
                {
                    row_buf[i] = COLOR_AV_GROUND;   /* 大地暖棕 */
                }
            }
            h += dh;
        }

        ST7735_DrawRGB16Span(x0, (uint16_t)(HORIZON_CY + dy), len, row_buf);
    }

    /* 3. 绘制动态俯仰梯形标尺刻度线 (+10°, +20°, -10°, -20°) */
    static const float s_pitch_ticks[4] = {-20.0f, -10.0f, 10.0f, 20.0f};
    for (int t = 0; t < 4; t++)
    {
        float p_mark = s_pitch_ticks[t];
        float H_mark = (p_mark - pitch_clamped) * PITCH_SCALE;

        /* 仅在圆盘可视区域内绘制 */
        if (fabsf(H_mark) > (float)(R - 6)) continue;

        /* 标尺中心坐标 */
        float bx = (float)HORIZON_CX - H_mark * sin_r;
        float by = (float)HORIZON_CY - H_mark * cos_r;

        /* 平行于地平线方向向量 */
        float ux = cos_r;
        float uy = -sin_r;

        /* 标尺半宽 5 像素 (全长 10 像素) */
        float bw = 5.0f;
        int16_t lx0 = (int16_t)roundf(bx - bw * ux);
        int16_t ly0 = (int16_t)roundf(by - bw * uy);
        int16_t lx1 = (int16_t)roundf(bx + bw * ux);
        int16_t ly1 = (int16_t)roundf(by + bw * uy);

        ST7735_DrawLine(lx0, ly0, lx1, ly1, COLOR_WHITE);

        /* 标尺向地平线方向弯折的端点倒勾 (2 像素) */
        float sign_dir = (p_mark > 0.0f) ? 1.0f : -1.0f;
        float tx = sign_dir * sin_r * 2.0f;
        float ty = sign_dir * cos_r * 2.0f;
        ST7735_DrawLine(lx0, ly0, (int16_t)roundf(lx0 + tx), (int16_t)roundf(ly0 + ty), COLOR_WHITE);
        ST7735_DrawLine(lx1, ly1, (int16_t)roundf(lx1 + tx), (int16_t)roundf(ly1 + ty), COLOR_WHITE);
    }

    /* 4. 绘制机体固定战机/飞鸟基准符号 (亮金黄色，指示飞机当前机体朝向) */
    /* 中心瞄准核 (3x3 像素) */
    ST7735_FillRect(HORIZON_CX - 1, HORIZON_CY - 1, 3, 3, COLOR_AV_AIRCRAFT);

    /* 左机翼横杠与向下外翼尖 (双层线条加粗显眼) */
    ST7735_DrawLine(HORIZON_CX - 13, HORIZON_CY,     HORIZON_CX - 5, HORIZON_CY,     COLOR_AV_AIRCRAFT);
    ST7735_DrawLine(HORIZON_CX - 13, HORIZON_CY + 1, HORIZON_CX - 5, HORIZON_CY + 1, COLOR_AV_AIRCRAFT);
    ST7735_DrawLine(HORIZON_CX - 5,  HORIZON_CY,     HORIZON_CX - 5, HORIZON_CY + 3, COLOR_AV_AIRCRAFT);

    /* 右机翼横杠与向下外翼尖 (双层线条加粗显眼) */
    ST7735_DrawLine(HORIZON_CX + 5,  HORIZON_CY,     HORIZON_CX + 13, HORIZON_CY,     COLOR_AV_AIRCRAFT);
    ST7735_DrawLine(HORIZON_CX + 5,  HORIZON_CY + 1, HORIZON_CX + 13, HORIZON_CY + 1, COLOR_AV_AIRCRAFT);
    ST7735_DrawLine(HORIZON_CX + 5,  HORIZON_CY,     HORIZON_CX + 5,  HORIZON_CY + 3, COLOR_AV_AIRCRAFT);

    /* 5. 外圈顶部零滚转指引三角标 (12 点钟方向白三角) */
    ST7735_DrawPixel(HORIZON_CX,     HORIZON_CY - R,     COLOR_WHITE);
    ST7735_DrawLine(HORIZON_CX - 1, HORIZON_CY - R - 1, HORIZON_CX + 1, HORIZON_CY - R - 1, COLOR_WHITE);
    ST7735_DrawLine(HORIZON_CX - 2, HORIZON_CY - R - 2, HORIZON_CX + 2, HORIZON_CY - R - 2, COLOR_WHITE);

    /* 外圈 ±30° 与 ±60° 倾角刻度短线 */
    ST7735_DrawLine(HORIZON_CX - 13, HORIZON_CY - 23, HORIZON_CX - 15, HORIZON_CY - 25, COLOR_WHITE);
    ST7735_DrawLine(HORIZON_CX + 13, HORIZON_CY - 23, HORIZON_CX + 15, HORIZON_CY - 25, COLOR_WHITE);
    ST7735_DrawLine(HORIZON_CX - 23, HORIZON_CY - 13, HORIZON_CX - 25, HORIZON_CY - 15, COLOR_WHITE);
    ST7735_DrawLine(HORIZON_CX + 23, HORIZON_CY - 13, HORIZON_CX + 25, HORIZON_CY - 15, COLOR_WHITE);

    s_last_pitch        = pitch;
    s_last_roll         = roll;
    s_has_prev_attitude = true;
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
