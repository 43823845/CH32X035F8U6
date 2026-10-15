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

/* PC17 (SW1) 微秒级脉冲探测采样:
 * 核心原理解析:
 * 1. 硬件电路: PC17 物理复用为 USB D+ (UDP)。物理按键 SW1 一端接 3.3V，经 4.7kΩ 电阻连至 PC17。
 * 2. 冲突根源: 系统启动 USB CDC 后，AFIO 开启片内 1.5kΩ 强上拉 (AFIO_CTLR_UDP_PUE)。
 *    即使按键释放，PC17 仍被片内 1.5kΩ 上拉维持在 ~3.0V 高电平；按键按下时为 3.07V~3.3V，
 *    导致单片机引脚电平永远为逻辑 1，按键释放态与按下态无法被常规 GPIO 采样区分！
 * 3. 时分微秒脉冲探测法:
 *    每 10ms 扫描周期内 (仅当 USB 非繁忙传输时)，短暂断开 AFIO 1.5kΩ 上拉 4 微秒：
 *    - 释放状态: PC17 失去 3.3V 来源，迅速经外部 USB 主机 15kΩ 下拉及片内泄放电阻泄放至 0V (逻辑 0)；
 *    - 按下状态: SW1 外部 4.7kΩ 电阻持续注入 3.3V，维持在 2.5V~3.3V 高电平 (逻辑 1)；
 *    采样完毕后立即恢复 AFIO 1.5kΩ 上拉。全过程仅 4us，对 USB 2.0 主机 2.0ms 的断开检测完全透明，
 *    既能 100% 灵敏、稳定捕获按键单击，又彻底保证 USB CDC 虚拟串口不丢包、不断线！
 */
static uint8_t Sample_PC17_Pin(void)
{
    /* 若 USB 正在收发数据包，跳过本次采样以确保 USB 总线绝对零扰动 */
    if (USB_CDC_IsBusy())
    {
        return 0xFF;
    }

    uint32_t old_afio;
    uint32_t old_cfgxr;
    uint8_t  pin_val;

    /* 关全局中断，确保 4us 微秒探针窗口原子执行，不受任何中断打断延长 */
    __disable_irq();

    old_afio  = AFIO->CTLR;
    old_cfgxr = GPIOC->CFGXR;

    /* 1. 临时断开片内 1.5kΩ USB 上拉 */
    AFIO->CTLR = old_afio & ~AFIO_CTLR_UDP_PUE;

    /* 2. 临时使能 1us 片内下拉泄放 (IPD)，加速线路电容向 0V 释放 */
    GPIOC->CFGXR = (old_cfgxr & ~((uint32_t)0x0F << 4)) | ((uint32_t)0x08 << 4);
    GPIOC->BSXR  = GPIO_BSXR_BR17;
    Delay_Us(1);

    /* 3. 转为浮空输入 (IN_FLOATING) 3us，消除下拉内阻对 4.7k 上拉的分压影响 */
    GPIOC->CFGXR = (old_cfgxr & ~((uint32_t)0x0F << 4)) | ((uint32_t)0x04 << 4);
    Delay_Us(3);

    /* 4. 稳态电平采样 (按下为 3.3V / SET，释放为 0V / RESET) */
    pin_val = (GPIO_ReadInputDataBit(KEY_PC17_PORT, KEY_PC17_PIN) != Bit_RESET) ? 1 : 0;

    /* 5. 立即恢复原始 GPIO 与 AFIO 1.5kΩ 上拉配置 */
    GPIOC->CFGXR = old_cfgxr;
    GPIOC->BSXR  = GPIO_BSXR_BS17;
    AFIO->CTLR   = old_afio;

    __enable_irq();

    return pin_val;
}

KeyEvent_t Key_PC17_Scan(void)
{
    static uint16_t s_pc17_press_count = 0;
    static uint8_t  s_pc17_pressed     = 0;
    KeyEvent_t event = KEY_EVENT_NONE;

    uint8_t pin_val = Sample_PC17_Pin();
    if (pin_val == 0xFF)
    {
        /* USB 正忙，跳过本 tick，保留当前消抖计数 */
        return KEY_EVENT_NONE;
    }

    if (pin_val == 1)
    {
        /* 硬件采样为高电平 (按键按下 3.3V) */
        if (s_pc17_press_count < 1000)
        {
            s_pc17_press_count++;
        }

        /* 消抖滤波：连续 2 个周期 (>= 20ms) 保持高电平，锁定为有效按下态 */
        if (s_pc17_press_count >= 2)
        {
            s_pc17_pressed = 1;
        }
    }
    else
    {
        /* 硬件采样为低电平 (按键释放 0V) */
        if (s_pc17_pressed)
        {
            /* 之前处于有效按下态，且按下时长在 20ms ~ 800ms 之间，判定为有效单击 */
            if (s_pc17_press_count >= 2 && s_pc17_press_count <= 80)
            {
                event = KEY_EVENT_CLICK;
            }
            s_pc17_pressed = 0;
        }
        s_pc17_press_count = 0;
    }

    return event;
}
