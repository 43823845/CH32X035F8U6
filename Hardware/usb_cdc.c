/********************************** (C) COPYRIGHT *******************************
 * File Name          : usb_cdc.c
 * Author             : WCH / Antigravity
 * Version            : V1.1.0
 * Description        : CH32X035 USB CDC 虚拟串口驱动实现 (官方例程对齐架构)
 *                      基于 EXAM/USB/USBFS/DEVICE/SimulateCDC 官方标准重构
 *******************************************************************************/
#include "usb_cdc.h"
#include "ch32x035_usb.h"
#include "ch32x035_pwr.h"
#include "debug.h"
#include <string.h>

#define CDC_MAX_PACKET_SIZE     64
#define CDC_TX_RING_SIZE        1024
#define CDC_RX_RING_SIZE        256

#define DEF_UEP_IN              0x80
#define DEF_UEP_OUT             0x00
#define DEF_UEP0                0x00
#define DEF_UEP1                0x01
#define DEF_UEP2                0x02
#define DEF_UEP3                0x03

/* CDC Line Coding 数据结构 (默认 115200 8N1) */
static uint8_t s_line_coding[7] = {
    0x00, 0xC2, 0x01, 0x00, /* 115200 波特率 (小端) */
    0x00,                   /* 1 停止位 */
    0x00,                   /* 无校验 */
    0x08                    /* 8 数据位 */
};

/* 设备状态与控制变量 */
static volatile uint8_t  s_dev_config     = 0;
static volatile uint8_t  s_dev_addr       = 0;
static volatile uint8_t  s_dev_sleep      = 0;
static volatile uint8_t  s_dev_enum       = 0;
static volatile uint8_t  s_ep_busy[4]     = {0};
static volatile bool     s_dtr_asserted   = false;

/* Setup 请求临时存储 */
static volatile uint8_t  s_setup_req_type = 0;
static volatile uint8_t  s_setup_req_code = 0;
static volatile uint16_t s_setup_req_val  = 0;
static volatile uint16_t s_setup_req_idx  = 0;
static volatile uint16_t s_setup_req_len  = 0;
static const uint8_t    *s_ep0_pdescr     = NULL;

/* 端点 DMA 缓冲区 (必须 4 字节对齐) */
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

/* ================== USB 描述符集合 (对齐官方标准) ================== */

/* 1. 设备描述符 (18 字节)
 * 官方标准：iSerialNumber = 0，避免 Windows 缓存固化序列号导致重插冲突与 Code 43 */
static const uint8_t s_device_descriptor[] = {
    0x12,                       /* bLength = 18 */
    0x01,                       /* bDescriptorType = Device */
    0x10, 0x01,                 /* bcdUSB = 1.10 */
    0x02,                       /* bDeviceClass = CDC (2) */
    0x00,                       /* bDeviceSubClass = 0 */
    0x00,                       /* bDeviceProtocol = 0 */
    CDC_MAX_PACKET_SIZE,        /* bMaxPacketSize0 = 64 */
    0x86, 0x1A,                 /* idVendor = 0x1A86 (WCH) */
    0x0C, 0xFE,                 /* idProduct = 0xFE0C (CDC) */
    0x01, 0x00,                 /* bcdDevice = 0.01 */
    0x01,                       /* iManufacturer = String 1 */
    0x02,                       /* iProduct = String 2 */
    0x00,                       /* iSerialNumber = 0 (无静态序列号，按端口动态分配) */
    0x01                        /* bNumConfigurations = 1 */
};

/* 2. 配置描述符集合 (总长 67 字节, 官方标准) */
static const uint8_t s_config_descriptor[] = {
    /* 配置描述符 (9 字节) */
    0x09, 0x02, 0x43, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,

    /* 接口 0: CDC 控制接口 (9 字节) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,

    /* CDC Functional: Header (5 字节) */
    0x05, 0x24, 0x00, 0x10, 0x01,

    /* CDC Functional: Call Management (5 字节) */
    0x05, 0x24, 0x01, 0x00, 0x01,

    /* CDC Functional: ACM (4 字节) */
    0x04, 0x24, 0x02, 0x02,

    /* CDC Functional: Union (5 字节) */
    0x05, 0x24, 0x06, 0x00, 0x01,

    /* 端点 1: 中断上传 (Interrupt IN, 7 字节) */
    0x07, 0x05, 0x81, 0x03, CDC_MAX_PACKET_SIZE, 0x00, 0x01,

    /* 接口 1: CDC 数据接口 (9 字节) */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

    /* 端点 2: 批量下发 (Bulk OUT, 7 字节) */
    0x07, 0x05, 0x02, 0x02, CDC_MAX_PACKET_SIZE, 0x00, 0x00,

    /* 端点 3: 批量上传 (Bulk IN, 7 字节) */
    0x07, 0x05, 0x83, 0x02, CDC_MAX_PACKET_SIZE, 0x00, 0x00,
};

/* 字符串描述符 0: 语言 ID (0x0409 英文) */
static const uint8_t s_string_langid[] = {
    0x04, 0x03, 0x09, 0x04
};

/* 字符串描述符 1: Manufacturer ("wch.cn") */
static const uint8_t s_string_vendor[] = {
    0x0E, 0x03,
    'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0
};

/* 字符串描述符 2: Product ("CH32X035 USB CDC") */
static const uint8_t s_string_product[] = {
    34, 0x03,
    'C', 0, 'H', 0, '3', 0, '2', 0, 'X', 0, '0', 0, '3', 0, '5', 0,
    ' ', 0, 'U', 0, 'S', 0, 'B', 0, ' ', 0, 'C', 0, 'D', 0, 'C', 0
};

/* ================== 底层端点与硬件初始化 ================== */

static void USBFS_Device_Endp_Init(void)
{
    USBFSD->UEP4_1_MOD = USBFS_UEP1_TX_EN;
    USBFSD->UEP2_3_MOD = USBFS_UEP2_RX_EN | USBFS_UEP3_TX_EN;

    USBFSD->UEP0_DMA = (uint32_t)s_ep0_buf;
    USBFSD->UEP1_DMA = (uint32_t)s_ep1_buf;
    USBFSD->UEP2_DMA = (uint32_t)s_ep2_buf;
    USBFSD->UEP3_DMA = (uint32_t)s_ep3_buf;

    USBFSD->UEP0_CTRL_H = USBFS_UEP_R_RES_ACK | USBFS_UEP_T_RES_NAK;
    USBFSD->UEP2_CTRL_H = USBFS_UEP_R_RES_ACK;

    USBFSD->UEP1_TX_LEN = 0;
    USBFSD->UEP3_TX_LEN = 0;

    USBFSD->UEP1_CTRL_H = USBFS_UEP_T_RES_NAK;
    USBFSD->UEP3_CTRL_H = USBFS_UEP_T_RES_NAK;

    s_ep_busy[0] = 0;
    s_ep_busy[1] = 0;
    s_ep_busy[2] = 0;
    s_ep_busy[3] = 0;
}

void USB_CDC_Init(void)
{
    /* 1. 开启 AFIO, GPIOC 与 USBFS 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBFS, ENABLE);

    /* 2. 官方标准引脚初始化: PC16(UDM/浮空输入), PC17(UDP/上拉输入) */
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_16;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_17;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* 3. 物理级软件断开重连:
     * 断开 UDP 内部 1.5kΩ 上拉，软件延时确保主机识别 SE0 掉线状态并清除旧枚举 */
    AFIO->CTLR &= ~(AFIO_CTLR_UDP_PUE | AFIO_CTLR_UDM_PUE | AFIO_CTLR_USB_IOEN);
    USBFSD->BASE_CTRL = USBFS_UC_RESET_SIE | USBFS_UC_CLR_ALL;
    for (volatile uint32_t i = 0; i < 400000; i++);
    USBFSD->BASE_CTRL = 0x00;

    /* 4. 根据当前供电电压配置 AFIO USB PHY 与内部 1.5kΩ 上拉电阻 (UDP_PUE_1K5 = 0x0C) */
    PWR_VDD VDD_Voltage = PWR_VDD_SupplyVoltage();
    if (VDD_Voltage == PWR_VDD_5V)
    {
        AFIO->CTLR = (AFIO->CTLR & ~(AFIO_CTLR_UDP_PUE | AFIO_CTLR_UDM_PUE | AFIO_CTLR_USB_PHY_V33)) 
                     | AFIO_CTLR_UDP_PUE_1 | AFIO_CTLR_USB_IOEN;
    }
    else
    {
        AFIO->CTLR = (AFIO->CTLR & ~(AFIO_CTLR_UDP_PUE | AFIO_CTLR_UDM_PUE)) 
                     | AFIO_CTLR_USB_PHY_V33 | AFIO_CTLR_UDP_PUE | AFIO_CTLR_USB_IOEN;
    }

    /* 5. 端点与内部状态初始化 */
    USBFS_Device_Endp_Init();

    s_dev_config   = 0;
    s_dev_addr     = 0;
    s_dev_sleep    = 0;
    s_dev_enum     = 0;
    s_dtr_asserted = false;
    s_tx_head      = 0;
    s_tx_tail      = 0;
    s_rx_head      = 0;
    s_rx_tail      = 0;

    USBFSD->DEV_ADDR  = 0x00;
    USBFSD->BASE_CTRL = USBFS_UC_DEV_PU_EN | USBFS_UC_INT_BUSY | USBFS_UC_DMA_EN;
    USBFSD->INT_FG    = 0xFF;
    USBFSD->UDEV_CTRL = USBFS_UD_PD_DIS | USBFS_UD_PORT_EN;
    USBFSD->INT_EN    = USBFS_UIE_SUSPEND | USBFS_UIE_BUS_RST | USBFS_UIE_TRANSFER;

    /* 6. 配置并使能 NVIC 中断 */
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    NVIC_InitStruct.NVIC_IRQChannel = USBFS_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

bool USB_CDC_IsConfigured(void)
{
    return (s_dev_config != 0);
}

bool USB_CDC_IsTerminalOpen(void)
{
    return s_dtr_asserted;
}

/* ================== 数据传输管理 (非阻塞环形缓冲) ================== */

static void USB_CDC_TriggerTx(void)
{
    if (s_dev_config == 0 || s_ep_busy[3]) return;

    if (s_tx_head != s_tx_tail)
    {
        uint16_t count = 0;
        while (s_tx_tail != s_tx_head && count < CDC_MAX_PACKET_SIZE)
        {
            s_ep3_buf[count++] = s_tx_ring[s_tx_tail];
            s_tx_tail = (s_tx_tail + 1) % CDC_TX_RING_SIZE;
        }

        if (count > 0)
        {
            s_ep_busy[3] = 1;
            USBFSD->UEP3_TX_LEN = count;
            USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_ACK;
        }
    }
}

uint32_t USB_CDC_SendBuffer(const uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return 0;

    NVIC_DisableIRQ(USBFS_IRQn);

    uint32_t written = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        uint16_t next_head = (s_tx_head + 1) % CDC_TX_RING_SIZE;
        if (next_head == s_tx_tail)
        {
            /* 环形队列满时丢弃最旧数据，确保最新调试日志不丢失且系统永不阻塞 */
            s_tx_tail = (s_tx_tail + 1) % CDC_TX_RING_SIZE;
        }
        s_tx_ring[s_tx_head] = buf[i];
        s_tx_head = next_head;
        written++;
    }

    if (s_dev_config != 0 && !s_ep_busy[3])
    {
        USB_CDC_TriggerTx();
    }

    NVIC_EnableIRQ(USBFS_IRQn);
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
    if (s_dev_config == 0) return;

    NVIC_DisableIRQ(USBFS_IRQn);
    if (!s_ep_busy[3])
    {
        USB_CDC_TriggerTx();
    }
    NVIC_EnableIRQ(USBFS_IRQn);
}

/* ================== USBFS 中断服务例程 (官方核心重构) ================== */

void USBFS_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USBFS_IRQHandler(void)
{
    uint8_t  intflag = USBFSD->INT_FG;
    uint8_t  intst   = USBFSD->INT_ST;
    uint16_t len     = 0;
    uint8_t  errflag = 0;

    if (intflag & USBFS_UIF_TRANSFER)
    {
        switch (intst & USBFS_UIS_TOKEN_MASK)
        {
            /* -------- 1. 数据上传完成阶段 (IN) -------- */
            case USBFS_UIS_TOKEN_IN:
                switch (intst & (USBFS_UIS_TOKEN_MASK | USBFS_UIS_ENDP_MASK))
                {
                    /* 端点 0 控制读上传 */
                    case USBFS_UIS_TOKEN_IN | DEF_UEP0:
                        if (s_setup_req_len == 0)
                        {
                            /* 握手状态阶段：准备接收 0 字节 DATA1 OUT 包 */
                            USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_R_RES_MASK) | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
                        }
                        if ((s_setup_req_type & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD)
                        {
                            switch (s_setup_req_code)
                            {
                                case USB_GET_DESCRIPTOR:
                                    len = (s_setup_req_len >= CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : s_setup_req_len;
                                    memcpy(s_ep0_buf, s_ep0_pdescr, len);
                                    s_setup_req_len -= len;
                                    s_ep0_pdescr += len;
                                    USBFSD->UEP0_TX_LEN  = len;
                                    USBFSD->UEP0_CTRL_H ^= USBFS_UEP_T_TOG;
                                    break;

                                case USB_SET_ADDRESS:
                                    USBFSD->DEV_ADDR = (USBFSD->DEV_ADDR & USBFS_UDA_GP_BIT) | s_dev_addr;
                                    break;

                                default:
                                    break;
                            }
                        }
                        break;

                    /* 端点 1 中断通知完成 */
                    case (USBFS_UIS_TOKEN_IN | DEF_UEP1):
                        USBFSD->UEP1_CTRL_H ^= USBFS_UEP_T_TOG;
                        USBFSD->UEP1_CTRL_H = (USBFSD->UEP1_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_NAK;
                        s_ep_busy[1] = 0;
                        break;

                    /* 端点 3 CDC 批量数据上传完成 */
                    case (USBFS_UIS_TOKEN_IN | DEF_UEP3):
                        USBFSD->UEP3_CTRL_H ^= USBFS_UEP_T_TOG;
                        USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_NAK;
                        s_ep_busy[3] = 0;
                        /* 链式触发下一包发送 */
                        USB_CDC_TriggerTx();
                        break;

                    default:
                        break;
                }
                break;

            /* -------- 2. 数据下发完成阶段 (OUT) -------- */
            case USBFS_UIS_TOKEN_OUT:
                switch (intst & (USBFS_UIS_TOKEN_MASK | USBFS_UIS_ENDP_MASK))
                {
                    /* 端点 0 控制写数据下发 */
                    case USBFS_UIS_TOKEN_OUT | DEF_UEP0:
                        if (intst & USBFS_UIS_TOG_OK)
                        {
                            if ((s_setup_req_type & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
                            {
                                s_setup_req_len = 0;
                                if (s_setup_req_code == CDC_SET_LINE_CODING)
                                {
                                    memcpy(s_line_coding, s_ep0_buf, 7);
                                    s_dtr_asserted = true;
                                }
                            }
                            if (s_setup_req_len == 0)
                            {
                                /* 状态阶段: 接收到控制写数据后向主机发送 0 字节 DATA1 IN 包确认 */
                                USBFSD->UEP0_TX_LEN  = 0;
                                USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
                            }
                        }
                        break;

                    /* 端点 2 CDC 批量数据接收 */
                    case (USBFS_UIS_TOKEN_OUT | DEF_UEP2):
                        USBFSD->UEP2_CTRL_H ^= USBFS_UEP_R_TOG;
                        len = USBFSD->RX_LEN;
                        for (uint16_t i = 0; i < len; i++)
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

                    default:
                        break;
                }
                break;

            /* -------- 3. 控制传输 SETUP 请求处理 -------- */
            case USBFS_UIS_TOKEN_SETUP:
            {
                USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_NAK | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_NAK;

                USB_SETUP_REQ *req = (USB_SETUP_REQ *)s_ep0_buf;
                s_setup_req_type = req->bRequestType;
                s_setup_req_code = req->bRequest;
                s_setup_req_len  = req->wLength;
                s_setup_req_val  = req->wValue;
                s_setup_req_idx  = req->wIndex;
                len = 0;
                errflag = 0;

                if ((s_setup_req_type & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
                {
                    /* CDC 类请求处理 */
                    if (s_setup_req_type & USB_REQ_TYP_CLASS)
                    {
                        switch (s_setup_req_code)
                        {
                            case CDC_GET_LINE_CODING:
                                s_ep0_pdescr = (uint8_t *)&s_line_coding[0];
                                len = 7;
                                break;

                            case CDC_SET_LINE_CODING:
                                break;

                            case CDC_SET_LINE_CTLSTE:
                                s_dtr_asserted = (s_setup_req_val & 0x01) != 0;
                                if (!s_dtr_asserted)
                                {
                                    /* 上位机关闭串口:
                                     * 1. 重置 EP3 为 NAK，清除 busy 标志，使主机驱动立即释放句柄，绝不卡死
                                     * 2. 清空发送环形队列，避免下一次打开时积压过时旧数据 */
                                    s_ep_busy[3] = 0;
                                    USBFSD->UEP3_TX_LEN = 0;
                                    USBFSD->UEP3_CTRL_H = (USBFSD->UEP3_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_RES_NAK;
                                    s_tx_head = 0;
                                    s_tx_tail = 0;
                                }
                                else
                                {
                                    /* 上位机打开串口: 尝试触发积压数据发送 */
                                    USB_CDC_TriggerTx();
                                }
                                break;

                            case CDC_SEND_BREAK:
                                break;

                            default:
                                errflag = 0xFF;
                                break;
                        }
                    }
                    else
                    {
                        errflag = 0xFF;
                    }

                    if (errflag == 0 && len > 0)
                    {
                        len = (s_setup_req_len >= CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : s_setup_req_len;
                        memcpy(s_ep0_buf, s_ep0_pdescr, len);
                        s_ep0_pdescr += len;
                    }
                }
                else
                {
                    /* USB 标准请求处理 */
                    switch (s_setup_req_code)
                    {
                        case USB_GET_DESCRIPTOR:
                            switch ((uint8_t)(s_setup_req_val >> 8))
                            {
                                case USB_DESCR_TYP_DEVICE:
                                    s_ep0_pdescr = s_device_descriptor;
                                    len = sizeof(s_device_descriptor);
                                    break;

                                case USB_DESCR_TYP_CONFIG:
                                    s_ep0_pdescr = s_config_descriptor;
                                    len = sizeof(s_config_descriptor);
                                    break;

                                case USB_DESCR_TYP_STRING:
                                    switch ((uint8_t)(s_setup_req_val & 0xFF))
                                    {
                                        case 0:
                                            s_ep0_pdescr = s_string_langid;
                                            len = sizeof(s_string_langid);
                                            break;
                                        case 1:
                                            s_ep0_pdescr = s_string_vendor;
                                            len = sizeof(s_string_vendor);
                                            break;
                                        case 2:
                                            s_ep0_pdescr = s_string_product;
                                            len = sizeof(s_string_product);
                                            break;
                                        default:
                                            errflag = 0xFF;
                                            break;
                                    }
                                    break;

                                default:
                                    errflag = 0xFF;
                                    break;
                            }

                            if (errflag == 0)
                            {
                                if (s_setup_req_len > len) s_setup_req_len = len;
                                len = (s_setup_req_len >= CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : s_setup_req_len;
                                memcpy(s_ep0_buf, s_ep0_pdescr, len);
                                s_ep0_pdescr += len;
                            }
                            break;

                        case USB_SET_ADDRESS:
                            s_dev_addr = (uint8_t)(s_setup_req_val & 0xFF);
                            break;

                        case USB_GET_CONFIGURATION:
                            s_ep0_buf[0] = s_dev_config;
                            if (s_setup_req_len > 1) s_setup_req_len = 1;
                            break;

                        case USB_SET_CONFIGURATION:
                            s_dev_config = (uint8_t)(s_setup_req_val & 0xFF);
                            s_dev_enum = 0x01;
                            USBFSD->UEP1_CTRL_H = USBFS_UEP_T_RES_NAK;
                            USBFSD->UEP2_CTRL_H = USBFS_UEP_R_RES_ACK;
                            USBFSD->UEP3_CTRL_H = USBFS_UEP_T_RES_NAK;
                            s_ep_busy[1] = 0;
                            s_ep_busy[2] = 0;
                            s_ep_busy[3] = 0;
                            break;

                        case USB_CLEAR_FEATURE:
                            if ((s_setup_req_type & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
                            {
                                switch ((uint8_t)(s_setup_req_idx & 0xFF))
                                {
                                    case (DEF_UEP_IN | DEF_UEP1):
                                        USBFSD->UEP1_CTRL_H = USBFS_UEP_T_RES_NAK;
                                        break;
                                    case (DEF_UEP_OUT | DEF_UEP2):
                                        USBFSD->UEP2_CTRL_H = USBFS_UEP_R_RES_ACK;
                                        break;
                                    case (DEF_UEP_IN | DEF_UEP3):
                                        USBFSD->UEP3_CTRL_H = USBFS_UEP_T_RES_NAK;
                                        s_ep_busy[3] = 0;
                                        break;
                                    default:
                                        errflag = 0xFF;
                                        break;
                                }
                            }
                            else
                            {
                                errflag = 0xFF;
                            }
                            break;

                        case USB_SET_FEATURE:
                            errflag = 0xFF;
                            break;

                        case USB_GET_INTERFACE:
                            s_ep0_buf[0] = 0x00;
                            if (s_setup_req_len > 1) s_setup_req_len = 1;
                            break;

                        case USB_SET_INTERFACE:
                            break;

                        case USB_GET_STATUS:
                            s_ep0_buf[0] = 0x00;
                            s_ep0_buf[1] = 0x00;
                            if (s_setup_req_len > 2) s_setup_req_len = 2;
                            break;

                        default:
                            errflag = 0xFF;
                            break;
                    }
                }

                /* 异常或不支持请求处理：严格 STALL 握手，决不能以空包冒充成功 */
                if (errflag == 0xFF)
                {
                    USBFSD->UEP0_CTRL_H = USBFS_UEP_T_TOG | USBFS_UEP_T_RES_STALL | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_STALL;
                }
                else
                {
                    if (s_setup_req_type & DEF_UEP_IN)
                    {
                        /* 控制读数据阶段 */
                        len = (s_setup_req_len > CDC_MAX_PACKET_SIZE) ? CDC_MAX_PACKET_SIZE : s_setup_req_len;
                        s_setup_req_len -= len;
                        USBFSD->UEP0_TX_LEN  = len;
                        USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
                    }
                    else
                    {
                        /* 控制写数据或状态阶段 */
                        if (s_setup_req_len == 0)
                        {
                            USBFSD->UEP0_TX_LEN  = 0;
                            USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_T_RES_MASK) | USBFS_UEP_T_TOG | USBFS_UEP_T_RES_ACK;
                        }
                        else
                        {
                            USBFSD->UEP0_CTRL_H = (USBFSD->UEP0_CTRL_H & ~USBFS_UEP_R_RES_MASK) | USBFS_UEP_R_TOG | USBFS_UEP_R_RES_ACK;
                        }
                    }
                }
                break;
            }

            default:
                break;
        }
        USBFSD->INT_FG = USBFS_UIF_TRANSFER;
    }
    else if (intflag & USBFS_UIF_BUS_RST)
    {
        /* 总线复位中断：重置所有配置与端点状态及发送/接收环形缓冲 */
        s_dev_config   = 0;
        s_dev_addr     = 0;
        s_dev_sleep    = 0;
        s_dev_enum     = 0;
        s_dtr_asserted = false;
        s_tx_head      = 0;
        s_tx_tail      = 0;
        s_rx_head      = 0;
        s_rx_tail      = 0;
        USBFSD->DEV_ADDR = 0;
        USBFS_Device_Endp_Init();
        USBFSD->INT_FG = USBFS_UIF_BUS_RST;
    }
    else if (intflag & USBFS_UIF_SUSPEND)
    {
        /* 总线挂起中断 */
        USBFSD->INT_FG = USBFS_UIF_SUSPEND;
    }
    else
    {
        USBFSD->INT_FG = intflag;
    }
}
