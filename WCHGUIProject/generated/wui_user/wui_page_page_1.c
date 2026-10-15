#include "wui_user/wui_page_page_1.h"

// ================= Key Handler =================

static void page_1_on_key(uint32_t key)
{
    (void)key;
}

// ================= Page Functions =================

void wui_page_page_1_init(void)
{
    wui_page_init(widgets_page_1, sizeof(widgets_page_1) / sizeof(widgets_page_1[0]));
    wui_register_key_handler(page_1_on_key);
}

void wui_page_page_1_loop(void)
{
}
