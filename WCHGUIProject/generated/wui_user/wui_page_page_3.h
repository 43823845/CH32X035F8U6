#ifndef WUI_PAGE_PAGE_3_H
#define WUI_PAGE_PAGE_3_H

#include "wui.h"
#include "wui_user/wui_page.h"
#include "wui_widget.h"
#include "images.h"
#include "wui_asset.h"
#include <stdio.h>

// ================= Widget ID Enum =================
typedef enum
{
    PAGE_3_COMPONENT_001,
    PAGE_3_COMPONENT_002,
    PAGE_3_COMPONENT_003,
    PAGE_3_COMPONENT_004,
} page_3_widget_id_t;

// ================= Declarative Widgets =================
static const wui_widget_desc_t widgets_page_3[] = {
    [PAGE_3_COMPONENT_001] = {
        .widget_id = PAGE_3_COMPONENT_001,
        .type      = Wui_w_StaticImage,
        .x         = 2,
        .y         = 12,
        .w         = 75,
        .h         = 22,
        .props.staticimg = {
            .img_id = IMAGE_TEXT_WUI_PAGE_3_COMPONENT_001,
        },
    },

    [PAGE_3_COMPONENT_002] = {
        .widget_id = PAGE_3_COMPONENT_002,
        .type      = Wui_w_StaticImage,
        .x         = 2,
        .y         = 48,
        .w         = 75,
        .h         = 22,
        .props.staticimg = {
            .img_id = IMAGE_TEXT_WUI_PAGE_3_COMPONENT_002,
        },
    },

    [PAGE_3_COMPONENT_003] = {
        .widget_id = PAGE_3_COMPONENT_003,
        .type      = Wui_w_SimpleString,
        .x         = 80,
        .y         = 12,
        .w         = 78,
        .h         = 22,
        .props.simplestring = {
            .str_id   = WUI_STR_NONE,
            .fg_color = RGB888_TO_RGB565(129, 199, 132),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_3_COMPONENT_004] = {
        .widget_id = PAGE_3_COMPONENT_004,
        .type      = Wui_w_SimpleString,
        .x         = 80,
        .y         = 48,
        .w         = 78,
        .h         = 22,
        .props.simplestring = {
            .str_id   = WUI_STR_NONE,
            .fg_color = RGB888_TO_RGB565(255, 183, 77),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

};


#endif // WUI_PAGE_PAGE_3_H
