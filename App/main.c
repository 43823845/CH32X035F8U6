/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH / Antigravity
 * Version            : V1.2.0
 * Description        : CH32X035F8U6 传感器与 WCHGUIProject 生成 UI 驱动主程序
 *******************************************************************************/
#include "debug.h"
#include "soft_i2c.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_backlight.h"
#include "bsp_int.h"
#include "st7735.h"
#include "usb_cdc.h"
#include "app_sensor.h"
#include "wui.h"
#include "wui_port.h"
#include "wui_page.h"
#include "wui_page_page_1.h"
#include "wui_page_page_2.h"
#include "wui_page_page_3.h"
#include "wui_page_page_4.h"

/* 用户显示控制逻辑 (实现位于 user_display_data.c) */
void user_display_attitude_update(float ax, float ay, float az);
void user_display_reset_cube(void);
void wui_page_page_2_update_data(float pitch, float roll,
                                 float ax, float ay, float az);
void wui_page_page_3_update_data(uint32_t tap_single, uint32_t tap_double,
                                 uint32_t ff_count, bool ff_recent);
void wui_page_page_4_update_data(float lux, uint8_t bl_percent);

/* 当前显示页, 上电默认 Page1 */
static wui_page_t s_cur_page = WUI_PAGE_1;

static void Handle_Key_Event(KeyEvent_t key)
{
    if (key == KEY_EVENT_CLICK)
    {
        /* 仅在 Page2/Page3/Page4 状态下响应循环切页: 开机 Page1 展示期按键无效 */
        if (s_cur_page == WUI_PAGE_2)
        {
            user_display_reset_cube();
            s_cur_page = WUI_PAGE_3;
            wui_page_switch((uint16_t)WUI_PAGE_3);
        }
        else if (s_cur_page == WUI_PAGE_3)
        {
            s_cur_page = WUI_PAGE_4;
            wui_page_switch((uint16_t)WUI_PAGE_4);
        }
        else if (s_cur_page == WUI_PAGE_4)
        {
            user_display_reset_cube();
            s_cur_page = WUI_PAGE_2;
            wui_page_switch((uint16_t)WUI_PAGE_2);
        }
    }
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    /* 1. 底层通信与硬件外设初始化 */
    Soft_I2C_Init();
    Bsp_Int_Init();
    LED_Init();
    Key_Init();
    Backlight_Init();
    ST7735_Init();
    USB_CDC_Init();

    printf("\r\n========================================\r\n");
    printf("  CH32X035F8U6 Sensor Monitor (INT1/2 Ready)  \r\n");
    printf("========================================\r\n");

    /* 2. 初始化 WCHGUI 框架并显示 Page1 (初始化欢迎页) */
    WUI_Port_Init();
    wui_page_switch(WUI_PAGE_1);

    /* 驱动初次渲染，展示开机画面 */
    for (int i = 0; i < 5; i++)
    {
        wui_page_ui_tick(10);
        wui_page_ui_update();
        wui_page_ui_loop();
        Delay_Ms(10);
    }

    /* 3. 系统硬件初始化与传感器自检 */
    App_Sensor_Init();

    /* 开机画面 (Page1) 停留展示约 2 秒 */
    for (int i = 0; i < 200; i++)
    {
        wui_page_ui_tick(10);
        wui_page_ui_update();
        wui_page_ui_loop();
        Delay_Ms(10);
    }

    /* 4. Page1 展示 2 秒后切换到 Page2 (传感器数据监控页) */
    s_cur_page = WUI_PAGE_2;
    wui_page_switch(WUI_PAGE_2);

    uint32_t tick_count = 0;

    while (1)
    {
        Delay_Ms(10);
        tick_count++;

        /* 驱动 WCHGUI 事件与刷新机制 */
        wui_page_ui_tick(10);
        wui_page_ui_update();
        wui_page_ui_loop();

        /* 10ms 任务: 按键扫描与交互 */
        KeyEvent_t key = Key_Scan();
        if (key != KEY_EVENT_NONE)
        {
            Handle_Key_Event(key);
        }

        /* 100ms 任务: 传感器采集并将光强、姿态、X,Y,Z 及背光数据更新到 Page2 控件 */
        if (tick_count % 10 == 0)
        {
            App_Sensor_Update();
            const App_SensorData_t *data = App_Sensor_GetData();

            /* 姿态判定每周期只更新一次, Page2/Page3 共用判定结果 */
            user_display_attitude_update(data->acc_x, data->acc_y, data->acc_z);

            /* 控件 ID 为页内序号, Page2/Page3/Page4 存在重号, 仅刷新当前活动页 */
            if (s_cur_page == WUI_PAGE_2)
            {
                wui_page_page_2_update_data(
                    data->pitch,
                    data->roll,
                    data->acc_x,
                    data->acc_y,
                    data->acc_z
                );
            }
            else if (s_cur_page == WUI_PAGE_3)
            {
                wui_page_page_3_update_data(
                    data->tap_single_count,
                    data->tap_double_count,
                    data->ff_count,
                    data->ff_recent
                );
            }
            else if (s_cur_page == WUI_PAGE_4)
            {
                wui_page_page_4_update_data(
                    data->lux,
                    Backlight_GetBrightness()
                );
            }

            /* 每 500ms 向上位机串口助手输出一次结构化调试信息 (定点数格式化，完美兼容 nano-printf) */
            if (tick_count % 50 == 0)
            {
                int32_t lux_i = (int32_t)data->lux;
                int32_t lux_d = (int32_t)((data->lux - (float)lux_i) * 10.0f);
                if (lux_d < 0) lux_d = -lux_d;

                int32_t pitch_i = (int32_t)data->pitch;
                int32_t roll_i  = (int32_t)data->roll;
                int32_t ax_i    = (int32_t)data->acc_x;
                int32_t ay_i    = (int32_t)data->acc_y;
                int32_t az_i    = (int32_t)data->acc_z;

                printf("[DEBUG] Lux:%ld.%ld | P:%ld R:%ld | X:%ld Y:%ld Z:%ld | BL:%d%% | TAP:%lu/%lu FF:%lu(%d) | INT1:%lu INT2:%lu\r\n",
                       (long)lux_i, (long)lux_d,
                       (long)pitch_i, (long)roll_i,
                       (long)ax_i, (long)ay_i, (long)az_i,
                       (int)Backlight_GetBrightness(),
                       (unsigned long)data->tap_single_count,
                       (unsigned long)data->tap_double_count,
                       (unsigned long)data->ff_count,
                       (int)data->ff_recent,
                       (unsigned long)data->int1_count,
                       (unsigned long)data->int2_count);
            }
        }

        /* 维护 USB CDC 发送队列 */
        USB_CDC_Poll();

        /* 500ms 任务: PB12 运行指示灯心跳闪烁 */
        if (tick_count % 50 == 0)
        {
            LED_Toggle();
        }
    }
}
