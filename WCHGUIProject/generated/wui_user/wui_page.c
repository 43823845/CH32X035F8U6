#include "wui_page.h"
#include "wui.h"
#include "images.h"
#include <stdio.h>


const wui_page_desc_t wui_pages[WUI_PAGE_COUNT] = {
    [WUI_PAGE_NONE] = {
        .bg_color = RGB888_TO_RGB565(0, 0, 0),
        .page_name = "None",
        .page_init = wui_page_none_init,
        .page_loop = wui_page_none_loop,
        .user_data = NULL,
    },
    [WUI_PAGE_1] = {
        .bg_color = RGB888_TO_RGB565(0, 0, 0),
        .page_name = "Page1",
        .page_init = wui_page_page_1_init,
        .page_loop = wui_page_page_1_loop,
        .user_data = NULL,
    },
    [WUI_PAGE_2] = {
        .bg_color = RGB888_TO_RGB565(0, 0, 0),
        .page_name = "Page2",
        .page_init = wui_page_page_2_init,
        .page_loop = wui_page_page_2_loop,
        .user_data = NULL,
    },
    [WUI_PAGE_3] = {
        .bg_color = RGB888_TO_RGB565(0, 0, 0),
        .page_name = "Page3",
        .page_init = wui_page_page_3_init,
        .page_loop = wui_page_page_3_loop,
        .user_data = NULL,
    },
    [WUI_PAGE_4] = {
        .bg_color = RGB888_TO_RGB565(0, 0, 0),
        .page_name = "Page4",
        .page_init = wui_page_page_4_init,
        .page_loop = wui_page_page_4_loop,
        .user_data = NULL,
    },
};
