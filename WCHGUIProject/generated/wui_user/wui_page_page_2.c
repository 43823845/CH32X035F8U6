#include "wui_page_page_2.h"
#include <stdio.h>
#include <string.h>

static char s_lux_str[24]   = "";
static char s_pose_str[24]  = "";
static char s_bl_str[24]    = "";
static char s_acc_x_str[16] = "";
static char s_acc_y_str[16] = "";
static char s_acc_z_str[16] = "";

// ================= Key Handler =================

static void page_2_on_key(uint32_t key)
{
    (void)key;
}

// ================= Page Functions =================

void wui_page_page_2_init(void)
{
    wui_page_init(widgets_page_2, sizeof(widgets_page_2) / sizeof(widgets_page_2[0]));
    wui_register_key_handler(page_2_on_key);
}

void wui_page_page_2_loop(void)
{
}

void wui_page_page_2_update_data(float lux, float pitch, float roll, float ax, float ay, float az, uint8_t bl_percent)
{
    char tmp[24];

    /* 1. 光强数值更新 (整型快速格式化，仅在内容变化时触发 UI 重绘) */
    snprintf(tmp, sizeof(tmp), "光强:%4d", (int)(lux + 0.5f));
    if (strcmp(s_lux_str, tmp) != 0)
    {
        memcpy(s_lux_str, tmp, sizeof(s_lux_str));
        wui_widget_simplestring_set_str(PAGE_2_COMPONENT_001, s_lux_str);
    }

    /* 2. 姿态角数值更新 */
    int p_int = (int)(pitch >= 0.0f ? pitch + 0.5f : pitch - 0.5f);
    int r_int = (int)(roll  >= 0.0f ? roll  + 0.5f : roll  - 0.5f);
    snprintf(tmp, sizeof(tmp), "姿态:P:%2d R:%2d", p_int, r_int);
    if (strcmp(s_pose_str, tmp) != 0)
    {
        memcpy(s_pose_str, tmp, sizeof(s_pose_str));
        wui_widget_simplestring_set_str(PAGE_2_COMPONENT_002, s_pose_str);
    }

    /* 3. 背光亮度更新 */
    snprintf(tmp, sizeof(tmp), "亮度:%3d%%", bl_percent);
    if (strcmp(s_bl_str, tmp) != 0)
    {
        memcpy(s_bl_str, tmp, sizeof(s_bl_str));
        wui_widget_simplestring_set_str(PAGE_2_COMPONENT_006, s_bl_str);
    }

    /* 4. X, Y, Z 三轴加速度更新 */
    int ax_int = (int)(ax >= 0.0f ? ax + 0.5f : ax - 0.5f);
    snprintf(tmp, sizeof(tmp), "X:%3d", ax_int);
    if (strcmp(s_acc_x_str, tmp) != 0)
    {
        memcpy(s_acc_x_str, tmp, sizeof(s_acc_x_str));
        wui_widget_simplestring_set_str(PAGE_2_COMPONENT_003, s_acc_x_str);
    }

    int ay_int = (int)(ay >= 0.0f ? ay + 0.5f : ay - 0.5f);
    snprintf(tmp, sizeof(tmp), "Y:%3d", ay_int);
    if (strcmp(s_acc_y_str, tmp) != 0)
    {
        memcpy(s_acc_y_str, tmp, sizeof(s_acc_y_str));
        wui_widget_simplestring_set_str(PAGE_2_COMPONENT_004, s_acc_y_str);
    }

    int az_int = (int)(az >= 0.0f ? az + 0.5f : az - 0.5f);
    snprintf(tmp, sizeof(tmp), "Z:%3d", az_int);
    if (strcmp(s_acc_z_str, tmp) != 0)
    {
        memcpy(s_acc_z_str, tmp, sizeof(s_acc_z_str));
        wui_widget_simplestring_set_str(PAGE_2_COMPONENT_005, s_acc_z_str);
    }
}
