#ifndef _APP_BTN_H_
#define _APP_BTN_H_

#include <zephyr/kernel.h>

#define BTN_BIT_MODE_WAITING BIT(0)
#define BTN_BIT_MODE_BASIC BIT(1) 
#define BTN_BIT_MODE_SERVER BIT(2)
#define BTN_BIT_MODE_SELECTED BIT(3)

extern struct k_event btn_mode_event;


void btn_init(void);

#endif /* _APP_BTN_H_ */
