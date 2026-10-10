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

static const uint8_t s_bl_levels[] = {100, 75, 50, 25, 10};
static uint8_t s_bl_index = 0;

static void Handle_Key_Event(KeyEvent_t key)
{
    if (key == KEY_EVENT_CLICK)
    {
        /* 单击循环调节背光档位 */
        s_bl_index = (s_bl_index + 1) % (sizeof(s_bl_levels) / sizeof(s_bl_levels[0]));
        Backlight_SetBrightness(s_bl_levels[s_bl_index]);
    }
    else if (key == KEY_EVENT_LONG_PRESS)
    {
        /* 长按开关背光 */
        if (Backlight_GetBrightness() > 0)
            Backlight_Off();
        else
            Backlight_SetBrightness(s_bl_levels[s_bl_index]);
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
    printf("  CH32X035 Sensor Monitor (INT1/2 Ready)  \r\n");
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

    /* 开机画面停留展示约 1.2 秒 */
    for (int i = 0; i < 120; i++)
    {
        wui_page_ui_tick(10);
        wui_page_ui_update();
        wui_page_ui_loop();
        Delay_Ms(10);
    }

    /* 4. 系统硬件正常后，切换到 Page2 (传感器数据监控页) */
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
            wui_page_page_2_update_data(
                data->lux,
                data->pitch,
                data->roll,
                data->acc_x,
                data->acc_y,
                data->acc_z,
                Backlight_GetBrightness()
            );

            /* 每 500ms 向上位机串口助手输出一次结构化调试信息 */
            if (tick_count % 50 == 0)
            {
                printf("[DEBUG] Lux:%6.1f | P:%3.0f R:%3.0f | X:%4.0f Y:%4.0f Z:%4.0f | BL:%3d%% | INT1:%lu INT2:%lu\r\n",
                       data->lux, data->pitch, data->roll,
                       data->acc_x, data->acc_y, data->acc_z,
                       Backlight_GetBrightness(),
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
