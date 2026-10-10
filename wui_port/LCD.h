#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>

#define LCD_SWAP16(x)             ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))
#define RGB888_TO_RGB565(r, g, b) LCD_SWAP16((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))
#define RGB24_TO_RGB565(rgb)      LCD_SWAP16((((rgb) >> 8) & 0xF800) | (((rgb) >> 5) & 0x07E0) | (((rgb) >> 3) & 0x001F))

#define LCD_WIDTH                 160
#define LCD_HEIGHT                80

#define LCD_WHITE                 RGB888_TO_RGB565(255, 255, 255)
#define LCD_BLACK                 RGB888_TO_RGB565(0, 0, 0)
#define LCD_white                 LCD_WHITE
#define LCD_black                 LCD_BLACK
#define LCD_BLUE                  RGB888_TO_RGB565(0, 0, 255)
#define LCD_RED                   RGB888_TO_RGB565(255, 0, 0)
#define LCD_GREEN                 RGB888_TO_RGB565(0, 255, 0)
#define LCD_YELLOW                RGB888_TO_RGB565(255, 255, 0)
#define LCD_CYAN                  RGB888_TO_RGB565(0, 255, 255)
#define LCD_MAGENTA               RGB888_TO_RGB565(255, 0, 255)
#define LCD_GRAY                  RGB888_TO_RGB565(128, 128, 128)

#endif /* __LCD_H */
