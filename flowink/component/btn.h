#ifndef _BTN_H_
#define _BTN_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#define BTN_BIT_MODE_BASIC BIT(0) 
#define BTN_BIT_MODE_SERVER BIT(1)
#define BTN_BIT_MODE_SELECTED BIT(2)
#define BTN_BIT_MODE_ALL (BTN_BIT_MODE_BASIC | BTN_BIT_MODE_SERVER | BTN_BIT_MODE_SELECTED)

extern struct k_event btn_mode_event;


void btn_init(void);

#endif
