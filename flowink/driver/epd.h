#ifndef _EPD_H_
#define _EPD_H_

#include <zephyr/types.h>


#define EPD_COLOR_BLACK   0x0   
#define EPD_COLOR_WHITE   0x1   
#define EPD_COLOR_YELLOW  0x2   
#define EPD_COLOR_RED     0x3   
#define EPD_COLOR_BLUE    0x5   
#define EPD_COLOR_GREEN   0x6   

void epd_wakeup_or_init(void);
void epd_fill_color(uint8_t color);
void epd_fill_image(uint8_t *Image);
void epd_sleep(void);

#endif
