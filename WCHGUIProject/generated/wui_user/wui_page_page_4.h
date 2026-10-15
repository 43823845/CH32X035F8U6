#ifndef WUI_PAGE_PAGE_4_H
#define WUI_PAGE_PAGE_4_H

#include "wui.h"
#include "wui_user/wui_page.h"
#include "wui_widget.h"
#include "images.h"
#include "wui_asset.h"
#include <stdio.h>

// ================= Widget ID Enum =================
typedef enum
{
    PAGE_4_COMPONENT_002,
    PAGE_4_COMPONENT_001,
} page_4_widget_id_t;

// ================= Declarative Widgets =================
static const wui_widget_desc_t widgets_page_4[] = {
    [PAGE_4_COMPONENT_002] = {
        .widget_id = PAGE_4_COMPONENT_002,
        .type      = Wui_w_SimpleString,
        .x         = 12,
        .y         = 45,
        .w         = 140,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_4_COMPONENT_002,
            .fg_color = RGB888_TO_RGB565(129, 199, 132),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_4_COMPONENT_001] = {
        .widget_id = PAGE_4_COMPONENT_001,
        .type      = Wui_w_SimpleString,
        .x         = 12,
        .y         = 13,
        .w         = 140,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_4_COMPONENT_001,
            .fg_color = RGB888_TO_RGB565(129, 199, 132),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

};


#endif // WUI_PAGE_PAGE_4_H
