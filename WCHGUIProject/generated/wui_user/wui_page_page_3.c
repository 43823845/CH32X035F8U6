#include "wui_user/wui_page_page_3.h"

// ================= Key Handler =================

static void page_3_on_key(uint32_t key)
{
    (void)key;
}

// ================= Page Functions =================

void wui_page_page_3_init(void)
{
    wui_page_init(widgets_page_3, sizeof(widgets_page_3) / sizeof(widgets_page_3[0]));
    wui_register_key_handler(page_3_on_key);
}

void wui_page_page_3_loop(void)
{
}
