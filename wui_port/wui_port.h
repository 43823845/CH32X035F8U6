#ifndef __WUI_PORT_H
#define __WUI_PORT_H

#include <stdint.h>
#include "wui.h"

void WUI_Port_Init(void);
const wui_hal_t* wui_get_hal(void);
void WUI_Port_SendKeyEvent(uint32_t key_code);

#endif /* __WUI_PORT_H */
