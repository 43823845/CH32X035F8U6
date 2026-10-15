#include "wui_user/wui_page_page_2.h"

// ================= Key Handler =================

static void page_2_on_key(uint32_t key)
{
    (void)key;
}

// ================= Page Functions =================

void wui_page_page_2_init(void)
{
    wui_page_init(widgets_page_2, sizeof(widgets_page_2) / sizeof(widgets_page_2[0]));
    wui_register_key_handler(page_2_on_key);
}

void wui_page_page_2_loop(void)
{
}
