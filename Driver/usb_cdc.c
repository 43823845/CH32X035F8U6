/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_cdc.c
 * Author             : WCH / Antigravity
 * Version            : V1.0.0
 * Description        : CH32X035 USB CDC 虚拟串口驱动实现 (Type-C 调试输出)
 *******************************************************************************/
#include "usb_cdc.h"
#include "ch32x035_usb.h"
#include <string.h>

#define CDC_MAX_PACKET_SIZE     64
#define CDC_TX_RING_SIZE        512
#define CDC_RX_RING_SIZE        256

/* Line Coding 数据结构 (默认 115200 8N1) */
static uint8_t s_line_coding[7] = {
    0x00, 0xC2, 0x01, 0x00, /* 115200 波特率 (小端) */
    0x00,                   /* 1 停止位 */
    0x00,                   /* 无校验 */
    0x08                    /* 8 数据位 */
};

/* 状态变量 */
static volatile bool     s_is_configured = false;
static volatile bool     s_ep3_tx_busy   = false;
static volatile uint8_t  s_dev_addr      = 0;
static volatile uint8_t  s_setup_req     = 0;

/* 端点 DMA 缓冲区 (4字节对齐) */
__attribute__((aligned(4))) static uint8_t s_ep0_buf[CDC_MAX_PACKET_SIZE];
__attribute__((aligned(4))) static uint8_t s_ep1_buf[CDC_MAX_PACKET_SIZE];
__attribute__((aligned(4))) static uint8_t s_ep2_buf[CDC_MAX_PACKET_SIZE];
__attribute__((aligned(4))) static uint8_t s_ep3_buf[CDC_MAX_PACKET_SIZE];

/* 发送环形队列 */
static uint8_t  s_tx_ring[CDC_TX_RING_SIZE];
static volatile uint16_t s_tx_head = 0;
static volatile uint16_t s_tx_tail = 0;

/* 接收环形队列 */
static uint8_t  s_rx_ring[CDC_RX_RING_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/* USB CDC 设备描述符 */
static const uint8_t s_device_descriptor[] = {
    0x12,                       /* bLength */
    0x01,                       /* bDescriptorType (Device) */
    0x10, 0x01,                 /* bcdUSB 1.10 */
    0x02,                       /* bDeviceClass (CDC) */
    0x00,                       /* bDeviceSubClass */
    0x00,                       /* bDeviceProtocol */
    CDC_MAX_PACKET_SIZE,        /* bMaxPacketSize0 */
    0x86, 0x1A,                 /* idVendor (0x1A86 - WCH) */
    0x0C, 0xFE,                 /* idProduct (0xFE0C - CDC) */
    0x00, 0x01,                 /* bcdDevice 1.00 */
    0x01,                       /* iManufacturer (String 1) */
    0x02,                       /* iProduct (String 2) */
    0x03,                       /* iSerialNumber (String 3) */
    0x01                        /* bNumConfigurations */
};

/* USB CDC 配置描述符集合 (总长 67 字节) */
static const uint8_t s_config_descriptor[] = {
    /* 1. Configuration Descriptor (9 bytes) */
    0x09, 0x02, 67, 0x00, 0x02, 0x01, 0x00, 0x80, 50,

    /* 2. Interface 0: CDC Communication Interface (9 bytes) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,

    /* 3. CDC Header Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x00, 0x10, 0x01,

    /* 4. CDC Call Management Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x01, 0x00, 0x01,

    /* 5. CDC ACM Functional Descriptor (4 bytes) */
    0x04, 0x24, 0x02, 0x02,

    /* 6. CDC Union Functional Descriptor (5 bytes) */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* 7. Endpoint 1: Interrupt IN (7 bytes) */
    0x07, 0x05, 0x81, 0x03, CDC_MAX_PACKET_SIZE, 0x00, 0x0A,

    /* 8. Interface 1: CDC Data Interface (9 bytes) */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

    /* 9. Endpoint 2: Bulk OUT (7 bytes) */
    0x07, 0x05, 0x02, 0x02, CDC_MAX_PACKET_SIZE, 0x00, 0x00,

    /* 10. Endpoint 3: Bulk IN (7 bytes) */
    0x07, 0x05, 0x83, 0x02, CDC_MAX_PACKET_SIZE, 0x00, 0x00,
};

/* 字符串描述符 0: 语言 ID (0x0409 英文) */
static const uint8_t s_string_langid[] = {
    0x04, 0x03, 0x09, 0x04
};

/* 字符串描述符 1: Manufacturer ("WCH") */
static const uint8_t s_string_vendor[] = {
    0x08, 0x03,
    'W', 0, 'C', 0, 'H', 0
};

/* 字符串描述符 2: Product ("CH32X035 CDC Debug Port") */
static const uint8_t s_string_product[] = {
    48, 0x03,
    'C', 0, 'H', 0, '3', 0, '2', 0, 'X', 0, '0', 0, '3', 0, '5', 0,
    ' ', 0, 'C', 0, 'D', 0, 'C', 0, ' ', 0,
    'D', 0, 'e', 0, 'b', 0, 'u', 0, 'g', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0
};

/* 字符串描述符 3: Serial ("20261010") */
static const uint8_t s_string_serial[] = {
    18, 0x03,
    '2', 0, '0', 0, '2', 0, '6', 0, '1', 0, '0', 0, '1', 0, '0', 0
};

static void USBFS_Device_Reset(void)
{
    USBFSD->DEV_ADDR    = 0x00;
    USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_NAK;
    USBFSD->UEP1_CTRL_H = USBFS_UEP_T_RES_NAK | USBFS_UEP_T_AUTO_TOG;
    USBFSD->UEP2_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_R_AUTO_TOG;
    USBFSD->UEP3_CTRL_H = USBFS_UEP_T_RES_NAK | USBFS_UEP_T_AUTO_TOG;

    s_is_configured = false;
    s_ep3_tx_busy   = false;
    s_dev_addr      = 0;
}

void USB_CDC_Init(void)
{
    /* 1. 开启 AFIO, GPIOC 与 USBFS 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS, ENABLE);

    /* 2. 配置 AFIO 控制器: 启用 USB PHY, 3.3V 供电并使能 PC16/PC17 内部 UDP 上拉 */
    AFIO->CTLR |= AFIO_CTLR_USB_PHY_V33 | AFIO_CTLR_USB_IOEN;
    AFIO->CTLR = (AFIO->CTLR & ~AFIO_CTLR_UDP_PUE) | AFIO_CTLR_UDP_PUE_1;

    /* 3. 初始化 USBFS 控制器与端点 DMA 地址 */
    USBFSD->BASE_CTRL = USBFS_UC_RESET_SIE | USBFS_UC_CLR_ALL;
    for (volatile int i = 0; i < 1000; i++);
    USBFSD->BASE_CTRL = 0x00;

    USBFSD->UEP0_DMA = (uint32_t)s_ep0_buf;
    USBFSD->UEP1_DMA = (uint32_t)s_ep1_buf;
    USBFSD->UEP2_DMA = (uint32_t)s_ep2_buf;
    USBFSD->UEP3_DMA = (uint32_t)s_ep3_buf;

    USBFSD->UEP4_1_MOD = USBFS_UEP1_TX_EN;
    USBFSD->UEP2_3_MOD = USBFS_UEP2_RX_EN | USBFS_UEP3_TX_EN;

    USBFS_Device_Reset();

    USBFSD->UDEV_CTRL = USBFS_UD_PD_DIS | USBFS_UD_PORT_EN;
    USBFSD->BASE_CTRL = USBFS_UC_DEV_PU_EN | USBFS_UC_INT_BUSY | USBFS_UC_DMA_EN;
    USBFSD->INT_EN    = USBFS_UIE_BUS_RST | USBFS_UIE_TRANSFER | USBFS_UIE_SUSPEND;

    /* 4. 配置并使能 NVIC 中断 */
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    NVIC_InitStruct.NVIC_IRQChannel = USBFS_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

bool USB_CDC_IsConfigured(void)
{
    return s_is_configured;
}

static void USB_CDC_TriggerTx(void)
{
    if (s_ep3_tx_busy || !s_is_configured) return;

    if (s_tx_head != s_tx_tail)
    {
        uint16_t count = 0;
        while (s_tx_tail != s_tx_head && count < CDC_MAX_PACKET_SIZE)
        {
            s_ep3_buf[count++] = s_tx_ring[s_tx_tail];
            s_tx_tail = (s_tx_tail + 1) % CDC_TX_RING_SIZE;
        }

        s_ep3_tx_busy = true;
        USBFSD->UEP3_TX_LEN = count;
        USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_ACK;
    }
}

uint32_t USB_CDC_SendBuffer(const uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return 0;

    uint32_t written = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        uint16_t next_head = (s_tx_head + 1) % CDC_TX_RING_SIZE;
        if (next_head == s_tx_tail)
        {
            /* 发送队列满，尝试立即触发发送以清空队列 */
            USB_CDC_TriggerTx();
            break;
        }
        s_tx_ring[s_tx_head] = buf[i];
        s_tx_head = next_head;
        written++;
    }

    USB_CDC_TriggerTx();
    return written;
}

void USB_CDC_SendString(const char *str)
{
    if (str)
    {
        USB_CDC_SendBuffer((const uint8_t *)str, strlen(str));
    }
}

uint32_t USB_CDC_Available(void)
{
    return (s_rx_head >= s_rx_tail) ? 
           (s_rx_head - s_rx_tail) : 
           (CDC_RX_RING_SIZE - s_rx_tail + s_rx_head);
}

int16_t USB_CDC_ReadByte(void)
{
    if (s_rx_head == s_rx_tail) return -1;
    uint8_t ch = s_rx_ring[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1) % CDC_RX_RING_SIZE;
    return ch;
}

void USB_CDC_Poll(void)
{
    USB_CDC_TriggerTx();
}

/* ================== USBFS 中断服务例程 ================== */

void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USBFS_IRQHandler(void)
{
    uint8_t int_fg = USBFSD->INT_FG;
    uint8_t int_st = USBFSD->INT_ST;

    /* 1. 总线复位中断 */
    if (int_fg & USBFS_UIF_BUS_RST)
    {
        USBFS_Device_Reset();
        USBFSD->INT_FG = USBFS_UIF_BUS_RST;
        return;
    }

    /* 2. 挂起/恢复中断 */
    if (int_fg & USBFS_UIF_SUSPEND)
    {
        USBFSD->INT_FG = USBFS_UIF_SUSPEND;
        return;
    }

    /* 3. 传输完成中断 */
    if (int_fg & USBFS_UIF_TRANSFER)
    {
        uint8_t endp  = int_st & USBFS_UIS_ENDP_MASK;
        uint8_t token = int_st & USBFS_UIS_TOKEN_MASK;

        switch (endp)
        {
            /* -------- 端点 0: 控制传输 -------- */
            case 0:
            {
                if (token == USBFS_UIS_TOKEN_SETUP)
                {
                    /* 读取 8 字节 Setup 包 */
                    USB_SETUP_REQ *req = (USB_SETUP_REQ *)s_ep0_buf;
                    s_setup_req = req->bRequest;
                    uint16_t len = req->wLength;
                    uint8_t *p_data = NULL;
                    uint16_t tx_len = 0;

                    if ((req->bRequestType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD)
                    {
                        switch (req->bRequest)
                        {
                            case USB_GET_DESCRIPTOR:
                            {
                                uint8_t desc_type  = (uint8_t)(req->wValue >> 8);
                                uint8_t desc_index = (uint8_t)(req->wValue & 0xFF);

                                switch (desc_type)
                                {
                                    case 0x01: /* Device Descriptor */
                                        p_data = (uint8_t *)s_device_descriptor;
                                        tx_len = sizeof(s_device_descriptor);
                                        break;
                                    case 0x02: /* Configuration Descriptor */
                                        p_data = (uint8_t *)s_config_descriptor;
                                        tx_len = sizeof(s_config_descriptor);
                                        break;
                                    case 0x03: /* String Descriptor */
                                        switch (desc_index)
                                        {
                                            case 0: p_data = (uint8_t *)s_string_langid;  tx_len = sizeof(s_string_langid);  break;
                                            case 1: p_data = (uint8_t *)s_string_vendor;  tx_len = sizeof(s_string_vendor);  break;
                                            case 2: p_data = (uint8_t *)s_string_product; tx_len = sizeof(s_string_product); break;
                                            case 3: p_data = (uint8_t *)s_string_serial;  tx_len = sizeof(s_string_serial);  break;
                                            default: break;
                                        }
                                        break;
                                    default: break;
                                }
                                break;
                            }

                            case USB_SET_ADDRESS:
                                s_dev_addr = (uint8_t)(req->wValue & 0x7F);
                                break;

                            case USB_SET_CONFIGURATION:
                                s_is_configured = (req->wValue != 0);
                                break;

                            case USB_GET_CONFIGURATION:
                                s_ep0_buf[0] = s_is_configured ? 1 : 0;
                                p_data = s_ep0_buf;
                                tx_len = 1;
                                break;

                            case USB_CLEAR_FEATURE:
                            case USB_SET_FEATURE:
                                break;

                            default:
                                break;
                        }
                    }
                    else if ((req->bRequestType & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS)
                    {
                        /* CDC 特殊类请求 */
                        switch (req->bRequest)
                        {
                            case CDC_GET_LINE_CODING:
                                p_data = s_line_coding;
                                tx_len = sizeof(s_line_coding);
                                break;
                            case CDC_SET_LINE_CODING:
                                /* 准备在下一个 OUT 数据阶段接收 Line Coding */
                                break;
                            case CDC_SET_LINE_CTLSTE:
                                /* 控制线状态改变 (DTR/RTS) */
                                break;
                            default:
                                break;
                        }
                    }

                    if (p_data != NULL)
                    {
                        if (tx_len > len) tx_len = len;
                        if (tx_len > CDC_MAX_PACKET_SIZE) tx_len = CDC_MAX_PACKET_SIZE;
                        memcpy(s_ep0_buf, p_data, tx_len);
                        USBFSD->UEP0_TX_LEN = tx_len;
                        USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_ACK | USBFS_UEP_T_TOG;
                    }
                    else
                    {
                        USBFSD->UEP0_TX_LEN = 0;
                        USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_ACK | USBFS_UEP_T_TOG;
                    }
                }
                else if (token == USBFS_UIS_TOKEN_IN)
                {
                    if (s_dev_addr != 0 && USBFSD->DEV_ADDR == 0)
                    {
                        USBFSD->DEV_ADDR = s_dev_addr;
                    }
                    USBFSD->UEP0_TX_LEN = 0;
                    USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_NAK;
                }
                else if (token == USBFS_UIS_TOKEN_OUT)
                {
                    if (s_setup_req == CDC_SET_LINE_CODING)
                    {
                        memcpy(s_line_coding, s_ep0_buf, sizeof(s_line_coding));
                    }
                    USBFSD->UEP0_TX_LEN = 0;
                    USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_ACK;
                }
                break;
            }

            /* -------- 端点 1: CDC Interrupt Notification (IN) -------- */
            case 1:
                USBFSD->UEP1_CTRL_H = (USBFSD->UEP1_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_NAK;
                break;

            /* -------- 端点 2: CDC Data (Bulk OUT) -------- */
            case 2:
            {
                uint16_t rx_len = USBFSD->RX_LEN;
                for (uint16_t i = 0; i < rx_len; i++)
                {
                    uint16_t next_head = (s_rx_head + 1) % CDC_RX_RING_SIZE;
                    if (next_head != s_rx_tail)
                    {
                        s_rx_ring[s_rx_head] = s_ep2_buf[i];
                        s_rx_head = next_head;
                    }
                }
                USBFSD->UEP2_CTRL_H = (USBFSD->UEP2_CTRL_H & ~USBFS_UEP_R_RES_MASK) | USBFS_UEP_R_RES_ACK;
                break;
            }

            /* -------- 端点 3: CDC Data (Bulk IN) -------- */
            case 3:
            {
                s_ep3_tx_busy = false;
                USB_CDC_TriggerTx();
                if (!s_ep3_tx_busy)
                {
                    USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_NAK;
                }
                break;
            }

            default:
                break;
        }

        USBFSD->INT_FG = USBFS_UIF_TRANSFER;
    }
}
