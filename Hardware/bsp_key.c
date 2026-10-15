#include "bsp_key.h"
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

    /* 2. 初始化 PC17 (SW1 复用按键): 使能 GPIOC 时钟 */
    RCC_APB2PeriphClockCmd(KEY_PC17_RCC, ENABLE);
}

KeyEvent_t Key_Scan(void)
{
    static uint16_t press_count = 0;
    static uint8_t key_released = 1;
    KeyEvent_t event = KEY_EVENT_NONE;

    /* PA2 用户按键按下为低电平 (板载 10k 上拉) */
    if (GPIO_ReadInputDataBit(KEY_GPIO_PORT, KEY_PIN) == Bit_RESET)
    {
        if (press_count < 1000)
            press_count++;
        
        if (press_count > 60 && key_released) /* 约 600ms 触发长按 */
        {
            event = KEY_EVENT_LONG_PRESS;
            key_released = 0;
        }
    }
    else
    {
        /* 释放检测 */
        if (press_count >= 2 && press_count <= 60 && key_released)
        {
            event = KEY_EVENT_CLICK;
        }
        press_count = 0;
        key_released = 1;
    }

    return event;
}

KeyEvent_t Key_PC17_Scan(void)
{
    static uint8_t  s_pc17_integrator  = 0;
    static uint8_t  s_pc17_pressed     = 0;
    static uint16_t s_pc17_hold_ticks  = 0;
    static uint8_t  s_pc17_idle_level  = 0;
    static uint8_t  s_pc17_inited      = 0;
    KeyEvent_t event = KEY_EVENT_NONE;

    /* 1. 微秒级过采样多数表决滤波:
     * 连续采样 5 次 (每次间隔 8us，窗口跨度约 40us)。
     * USB 12Mbps 全速通信每个 bit 仅 83ns，40us 窗口覆盖约 500 个 bit 周期，
     * 多数表决算法可彻底抹除 USB 调试日志数据帧与 SOF 令牌带来的瞬态高频跳变！ */
    uint8_t high_samples = 0;
    for (uint8_t i = 0; i < 5; i++)
    {
        if (GPIO_ReadInputDataBit(KEY_PC17_PORT, KEY_PC17_PIN) != Bit_RESET)
        {
            high_samples++;
        }
        Delay_Us(8);
    }
    uint8_t curr = (high_samples >= 3) ? 1 : 0;

    /* 2. 首次上电自动捕获稳态基准电平 */
    if (!s_pc17_inited)
    {
        s_pc17_idle_level = curr;
        s_pc17_inited = 1;
        return KEY_EVENT_NONE;
    }

    /* 3. 迟滞漏桶积分器 (Leaky Integrator):
     * 电平偏离空闲基准时快速累加，恢复基准时逐步衰减。
     * 彻底解决简单线性计数器被偶发 USB 数据包瞬间“清零”而导致的按键不灵敏与漏检问题！ */
    if (curr != s_pc17_idle_level)
    {
        if (s_pc17_integrator < 8)
        {
            s_pc17_integrator++;
        }
        s_pc17_hold_ticks++;

        /* 持续超过 1.5 秒 (150 个 10ms 周期) 仍未恢复，判定为外部硬件常态变化(如USB线插拔改变了常态电平)，自适应迁移基准 */
        if (s_pc17_hold_ticks > 150)
        {
            s_pc17_idle_level = curr;
            s_pc17_integrator = 0;
            s_pc17_pressed    = 0;
            s_pc17_hold_ticks = 0;
        }
    }
    else
    {
        if (s_pc17_integrator > 0)
        {
            s_pc17_integrator--;
        }
        s_pc17_hold_ticks = 0;
    }

    /* 4. 双门限迟滞判决 (Hysteresis Decision):
     * 积分累积 >= 3 (人手有效按压约 30ms) 锁定进入按下态；
     * 积分衰减至 0 确认按键完全释放，立即触发一次灵敏、干净的单击事件！ */
    if (s_pc17_integrator >= 3)
    {
        s_pc17_pressed = 1;
    }
    else if (s_pc17_integrator == 0 && s_pc17_pressed)
    {
        s_pc17_pressed = 0;
        event = KEY_EVENT_CLICK;
    }

    return event;
}
