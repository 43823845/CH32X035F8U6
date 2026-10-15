#ifndef WUI_PAGE_PAGE_1_H
#define WUI_PAGE_PAGE_1_H

#include "wui.h"
#include "wui_user/wui_page.h"
#include "wui_widget.h"
#include "images.h"
#include "wui_asset.h"
#include <stdio.h>

// ================= Widget ID Enum =================
typedef enum
{
    PAGE_1_COMPONENT_001,
    PAGE_1_COMPONENT_003,
} page_1_widget_id_t;

// ================= Declarative Widgets =================
static const wui_widget_desc_t widgets_page_1[] = {
    [PAGE_1_COMPONENT_001] = {
        .widget_id = PAGE_1_COMPONENT_001,
        .type      = Wui_w_StaticImage,
        .x         = 46,
        .y         = 6,
        .w         = 73,
        .h         = 30,
        .props.staticimg = {
            .img_id = IMAGE_TEXT_WUI_PAGE_1_COMPONENT_001,
        },
    },

    [PAGE_1_COMPONENT_003] = {
        .widget_id = PAGE_1_COMPONENT_003,
        .type      = Wui_w_SimpleString,
        .x         = 14,
        .y         = 44,
        .w         = 134,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_1_COMPONENT_003,
            .fg_color = RGB888_TO_RGB565(79, 195, 247),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

};


#endif // WUI_PAGE_PAGE_1_H
