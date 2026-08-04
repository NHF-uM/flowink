#ifndef _EPD_H_
#define _EPD_H_

#include <zephyr/types.h>


#define EPD_COLOR_BLACK   0x0   
#define EPD_COLOR_WHITE   0x1   
#define EPD_COLOR_YELLOW  0x2   
#define EPD_COLOR_RED     0x3   
#define EPD_COLOR_BLUE    0x5   
#define EPD_COLOR_GREEN   0x6   

#define EPD_7IN3E_WIDTH     DT_PROP(EPD_NODE, width)
#define EPD_7IN3E_HEIGHT    DT_PROP(EPD_NODE, height)
#define EPD_SIZE_BYTE       ((EPD_7IN3E_WIDTH % 2 == 0 ? (EPD_7IN3E_WIDTH / 2) : (EPD_7IN3E_WIDTH / 2 + 1)) * EPD_7IN3E_HEIGHT)

void epd_reset(void);
void epd_init(void);
void epd_fill_color(uint8_t color);
void epd_fill_image(uint8_t *Image);
void epd_sleep(void);

#endif
