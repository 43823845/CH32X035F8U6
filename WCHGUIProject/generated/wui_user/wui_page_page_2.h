#ifndef WUI_PAGE_PAGE_2_H
#define WUI_PAGE_PAGE_2_H

#include "wui.h"
#include "wui_user/wui_page.h"
#include "wui_widget.h"
#include "images.h"
#include "wui_asset.h"
#include <stdio.h>

// ================= Widget ID Enum =================
typedef enum
{
    PAGE_2_COMPONENT_001,
    PAGE_2_COMPONENT_002,
    PAGE_2_COMPONENT_003,
    PAGE_2_COMPONENT_004,
    PAGE_2_COMPONENT_005,
} page_2_widget_id_t;

// ================= Declarative Widgets =================
static const wui_widget_desc_t widgets_page_2[] = {
    [PAGE_2_COMPONENT_001] = {
        .widget_id = PAGE_2_COMPONENT_001,
        .type      = Wui_w_SimpleString,
        .x         = 3,
        .y         = 0,
        .w         = 37,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_001,
            .fg_color = RGB888_TO_RGB565(129, 199, 132),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_002] = {
        .widget_id = PAGE_2_COMPONENT_002,
        .type      = Wui_w_SimpleString,
        .x         = 4,
        .y         = 20,
        .w         = 37,
        .h         = 19,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_002,
            .fg_color = RGB888_TO_RGB565(255, 183, 77),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_003] = {
        .widget_id = PAGE_2_COMPONENT_003,
        .type      = Wui_w_SimpleString,
        .x         = 4,
        .y         = 40,
        .w         = 37,
        .h         = 18,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_003,
            .fg_color = RGB888_TO_RGB565(255, 183, 77),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_004] = {
        .widget_id = PAGE_2_COMPONENT_004,
        .type      = Wui_w_SimpleString,
        .x         = 4,
        .y         = 58,
        .w         = 37,
        .h         = 18,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_004,
            .fg_color = RGB888_TO_RGB565(255, 183, 77),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_005] = {
        .widget_id = PAGE_2_COMPONENT_005,
        .type      = Wui_w_ProgressBar,
        .x         = 154,
        .y         = 5,
        .w         = 4,
        .h         = 74,
        .props.progress = {
            .direction     = WUI_PB_VERTICAL,
            .min_value     = 0,
            .max_value     = 100,
            .current_value = 20,
            .bg_color      = RGB888_TO_RGB565(0, 0, 0),
            .fill_color    = RGB888_TO_RGB565(129, 199, 132),
        },
    },

};


#endif // WUI_PAGE_PAGE_2_H
