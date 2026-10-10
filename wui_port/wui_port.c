#include "wui_port.h"
#include "st7735.h"
#include "spi_driver.h"
#include <string.h>

#include "wui_internal_data.h"

#define WUI_HEAP_SIZE   3072
static uint8_t s_wui_heap[WUI_HEAP_SIZE] __attribute__((aligned(4)));

static void port_touch_read(uint16_t* x, uint16_t* y, uint8_t* pressed)
{
    if (x) *x = 0;
    if (y) *y = 0;
    if (pressed) *pressed = 0;
}

static void port_flash_read(uint8_t* buf, uint32_t addr, uint32_t size)
{
    if (buf && size)
    {
        if (addr < sizeof(wui_internal_data))
        {
            uint32_t read_len = size;
            if (addr + read_len > sizeof(wui_internal_data))
            {
                read_len = sizeof(wui_internal_data) - addr;
            }
            memcpy(buf, &wui_internal_data[addr], read_len);
            if (read_len < size)
            {
                memset(buf + read_len, 0, size - read_len);
            }
        }
        else
        {
            memset(buf, 0, size);
        }
    }
}

static void port_lcd_set_windows(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    ST7735_SetAddressWindow(x1, y1, x2, y2);
}

static void port_lcd_dma_start(void)
{
    /* 设置 DC 为数据模式 */
    GPIO_SetBits(ST7735_DC_PORT, ST7735_DC_PIN);
}

static void port_lcd_dma_send(const uint8_t* buf, uint32_t len)
{
    SPI1_WriteBuffer(buf, (uint16_t)len);
}

static void port_lcd_dma_wait(void)
{
    /* SPI1_WriteBuffer 内已保证发送完成并等待 BSY 标志清除 */
}

static void port_lcd_dma_end(void)
{
    /* 传输结束处理 */
}

const wui_hal_t* wui_get_hal(void)
{
    static const wui_hal_t hal = {
        .touch_read      = port_touch_read,
        .flash_read      = port_flash_read,
        .lcd_set_windows = port_lcd_set_windows,
        .lcd_dma_start   = port_lcd_dma_start,
        .lcd_dma_send    = port_lcd_dma_send,
        .lcd_dma_wait    = port_lcd_dma_wait,
        .lcd_dma_end     = port_lcd_dma_end,
    };
    return &hal;
}

void WUI_Port_Init(void)
{
    wui_register_hal(wui_get_hal());

    wui_init_data_t init_data = {
        .width      = 160,
        .height     = 80,
        .page_count = 2,
        .heap_addr  = s_wui_heap,
        .heap_size  = WUI_HEAP_SIZE,
    };
    wui_init(&init_data);
}

void WUI_Port_SendKeyEvent(uint32_t key_code)
{
    wui_set_key_event(key_code);
}
