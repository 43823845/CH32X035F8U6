#include "bsp_key.h"
#include "usb_cdc.h"
#include "debug.h"

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. 初始化 PA2 (SW2 用户按键): 上拉输入 */
    RCC_APB2PeriphClockCmd(KEY_RCC_PORT, ENABLE);

    GPIO_InitStruct.GPIO_Pin = KEY_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU; /* 上拉输入 */
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_GPIO_PORT, &GPIO_InitStruct);

    /* 2. PC17 在硬件上与 USB D+ (UDP) 物理共用。
     * 为确保 USB CDC 虚拟串口 100% 稳定运行，不作破坏性模式重写。 */
    RCC_APB2PeriphClockCmd(KEY_PC17_RCC, ENABLE);
}

/**
 * @brief  PA2 (SW2) 用户按键扫描状态机
 *         支持单击、双击与长按三合一复合检测：
 *         - 单击 (Click): 循环切换当前监控页面 (Page2 -> Page3 -> Page4)
 *         - 双击 (Double Click): 循环切换 3 个背光亮度挡位 (100% -> 60% -> 20%)
 *         - 长按 (Long Press): 开启 / 关闭 LCD 屏幕背光电源
 * @return KeyEvent_t
 */
KeyEvent_t Key_Scan(void)
{
    static uint16_t s_press_count   = 0;
    static uint16_t s_click_timeout = 0;
    static uint8_t  s_click_pending = 0;
    static uint8_t  s_long_pressed  = 0;
    KeyEvent_t event = KEY_EVENT_NONE;

    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_PIN) == Bit_RESET)
    {
        /* 按键处于有效按下状态 (PA2 板载 10k 上拉，按下接地为低电平) */
        if (s_press_count < 1000)
        {
            s_press_count++;
        }

        /* 持续按下超过 600ms (60 个 10ms tick)，触发长按事件 */
        if (s_press_count > 60 && !s_long_pressed)
        {
            event = KEY_EVENT_LONG_PRESS;
            s_long_pressed  = 1;
            s_click_pending = 0; /* 长按触发后立即废弃单击/双击判定 */
        }
    }
    else
    {
        /* 按键处于释放状态 (高电平) */
        if (s_long_pressed)
        {
            s_long_pressed = 0;
            s_press_count  = 0;
        }
        else if (s_press_count >= 2 && s_press_count <= 60)
        {
            /* 有效短按释放 (持续时间 20ms ~ 600ms) */
            s_press_count = 0;
            if (s_click_pending)
            {
                /* 在双击判定超时窗口内再次发生短按释放 -> 判定为双击！ */
                event = KEY_EVENT_DOUBLE_CLICK;
                s_click_pending = 0;
                s_click_timeout = 0;
            }
            else
            {
                /* 记录第一次单击，开启约 250ms (25 个 10ms tick) 的双击等待窗口 */
                s_click_pending = 1;
                s_click_timeout = 25;
            }
        }
        else
        {
            s_press_count = 0;
        }

        /* 双击等待窗口倒计时处理 */
        if (s_click_pending)
        {
            if (s_click_timeout > 0)
            {
                s_click_timeout--;
            }
            else
            {
                /* 倒计时归零，用户未在窗口内再次按下，确认触发单击 */
                s_click_pending = 0;
                event = KEY_EVENT_CLICK;
            }
        }
    }

    return event;
}

/**
 * @brief  PC17 (SW1) 扫描接口
 *         说明: PC17 在硬件上直连 USB 2.0 全速 D+ 数据线。在 USB CDC 驱动使能后，
 *         物理层必须维持稳定的 1.5kΩ 上拉电平。为保障 USB 虚拟串口通信绝对稳定，
 *         严禁在运行期对 D+ 施加任何拉低扰动。
 */
KeyEvent_t Key_PC17_Scan(void)
{
    return KEY_EVENT_NONE;
}
