#ifndef _RGB_STRIP_H_
#define _RGB_STRIP_H_

#include <zephyr/kernel.h>

enum {
    RED = 0,
    GREEN,
    BLUE,
} ;

void rgb_strip_on(uint8_t color);
void rgb_strip_off(void);

#endif /* _RGB_STRIP_H_ */