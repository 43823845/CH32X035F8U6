#include "wui_page_page_2.h"
#include <stdio.h>

static char s_lux_str[24]   = "光强:0.0";
static char s_pose_str[24]  = "姿态:P:0 R:0";
static char s_bl_str[24]    = "亮度:100%";
static char s_acc_x_str[16] = "X:0";
static char s_acc_y_str[16] = "Y:0";
static char s_acc_z_str[16] = "Z:0";

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
    /* 1. 光强数值更新 */
    snprintf(s_lux_str, sizeof(s_lux_str), "光强:%4.0f", lux);
    wui_widget_simplestring_set_str(PAGE_2_COMPONENT_001, s_lux_str);

    /* 2. 姿态角数值更新 */
    snprintf(s_pose_str, sizeof(s_pose_str), "姿态:P:%2.0f R:%2.0f", pitch, roll);
    wui_widget_simplestring_set_str(PAGE_2_COMPONENT_002, s_pose_str);

    /* 3. 背光亮度更新 */
    snprintf(s_bl_str, sizeof(s_bl_str), "亮度:%3d%%", bl_percent);
    wui_widget_simplestring_set_str(PAGE_2_COMPONENT_006, s_bl_str);

    /* 4. X, Y, Z 三轴加速度更新 */
    snprintf(s_acc_x_str, sizeof(s_acc_x_str), "X:%3.0f", ax);
    wui_widget_simplestring_set_str(PAGE_2_COMPONENT_003, s_acc_x_str);

    snprintf(s_acc_y_str, sizeof(s_acc_y_str), "Y:%3.0f", ay);
    wui_widget_simplestring_set_str(PAGE_2_COMPONENT_004, s_acc_y_str);

    snprintf(s_acc_z_str, sizeof(s_acc_z_str), "Z:%3.0f", az);
    wui_widget_simplestring_set_str(PAGE_2_COMPONENT_005, s_acc_z_str);
}
