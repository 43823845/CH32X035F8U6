#ifndef WUI_PAGE_H
#define WUI_PAGE_H

#include <stdint.h>
#include "stddef.h"
#include "wui.h"

typedef enum
{
    WUI_PAGE_NONE = 0,
    WUI_PAGE_1,
    WUI_PAGE_2,
    WUI_PAGE_3,
    WUI_PAGE_4,
    WUI_PAGE_COUNT,
} wui_page_t;

#define WUI_START_PAGE WUI_PAGE_1

void wui_page_none_init(void);
void wui_page_none_loop(void);

void wui_page_page_1_init(void);
void wui_page_page_1_loop(void);

void wui_page_page_2_init(void);
void wui_page_page_2_loop(void);

void wui_page_page_3_init(void);
void wui_page_page_3_loop(void);

void wui_page_page_4_init(void);
void wui_page_page_4_loop(void);

extern const wui_page_desc_t wui_pages[WUI_PAGE_COUNT];

#endif
