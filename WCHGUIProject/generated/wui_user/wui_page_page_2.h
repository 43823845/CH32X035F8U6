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
    PAGE_2_COMPONENT_006,
} page_2_widget_id_t;

// ================= Declarative Widgets =================
static const wui_widget_desc_t widgets_page_2[] = {
    [PAGE_2_COMPONENT_001] = {
        .widget_id = PAGE_2_COMPONENT_001,
        .type      = Wui_w_SimpleString,
        .x         = 7,
        .y         = 3,
        .w         = 76,
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
        .x         = 7,
        .y         = 29,
        .w         = 148,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_002,
            .fg_color = RGB888_TO_RGB565(129, 199, 132),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_003] = {
        .widget_id = PAGE_2_COMPONENT_003,
        .type      = Wui_w_SimpleString,
        .x         = 7,
        .y         = 56,
        .w         = 46,
        .h         = 24,
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
        .x         = 57,
        .y         = 56,
        .w         = 46,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_004,
            .fg_color = RGB888_TO_RGB565(255, 183, 77),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_005] = {
        .widget_id = PAGE_2_COMPONENT_005,
        .type      = Wui_w_SimpleString,
        .x         = 107,
        .y         = 56,
        .w         = 48,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_005,
            .fg_color = RGB888_TO_RGB565(255, 183, 77),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

    [PAGE_2_COMPONENT_006] = {
        .widget_id = PAGE_2_COMPONENT_006,
        .type      = Wui_w_SimpleString,
        .x         = 87,
        .y         = 3,
        .w         = 68,
        .h         = 24,
        .props.simplestring = {
            .str_id   = WUI_STR_PAGE_2_COMPONENT_006,
            .fg_color = RGB888_TO_RGB565(129, 199, 132),
            .bg_color = RGB888_TO_RGB565(0, 0, 0),
            .align    = WUI_SIMPLESTRING_ALIGN_LEFT,
        },
    },

};


void wui_page_page_2_update_data(float lux, float pitch, float roll, float ax, float ay, float az, uint8_t bl_percent);

#endif // WUI_PAGE_PAGE_2_H
