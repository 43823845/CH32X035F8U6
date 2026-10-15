#include "wui_user/wui_page_page_4.h"

// ================= Key Handler =================

static void page_4_on_key(uint32_t key)
{
    (void)key;
}

// ================= Page Functions =================

void wui_page_page_4_init(void)
{
    wui_page_init(widgets_page_4, sizeof(widgets_page_4) / sizeof(widgets_page_4[0]));
    wui_register_key_handler(page_4_on_key);
}

void wui_page_page_4_loop(void)
{
}
